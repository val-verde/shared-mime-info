#include <config.h>

#define N_(x) x
#define _(x) (x)

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>
#include <dirent.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <sys/stat.h>
#include <sys/types.h>

#define XML_NS XML_XML_NAMESPACE
#define XMLNS_NS "http://www.w3.org/2000/xmlns/"
#define FREE_NS "http://www.freedesktop.org/standards/shared-mime-info"

#define COPYING								\
	     N_("Copyright (C) 2002 Thomas Leonard.\n"			\
		"update-mime-database comes with ABSOLUTELY NO WARRANTY,\n" \
		"to the extent permitted by law.\n"			\
		"You may redistribute copies of update-mime-database\n"	\
		"under the terms of the GNU General Public License.\n"	\
		"For more information about these matters, "		\
		"see the file named COPYING.\n")

const char *media_types[] = {
	"text",
	"application",
	"image",
	"audio",
	"inode",
	"video",
	"message",
	"model",
	"multipart",
};

typedef struct _Type Type;

struct _Type {
	char *media;
	char *subtype;

	/* contains xmlNodes for elements that are being copied to the output */
	xmlDoc	*output;
};

/* Maps MIME type names to Types */
static GHashTable *types = NULL;

/* Maps glob patterns to Types */
static GHashTable *globs_hash = NULL;

/* 'magic' nodes */
static GPtrArray *magic = NULL;

static void usage(const char *name)
{
	fprintf(stderr, _("Usage: %s [-hv] MIME-DIR\n"), name);
}

static void free_type(gpointer data)
{
	Type *type = (Type *) data;

	g_free(type->media);
	g_free(type->subtype);

	xmlFreeDoc(type->output);

	g_free(type);
}

static Type *get_type(const char *name)
{
	xmlNode *root;
	xmlNs *ns;
	const char *slash;
	Type *type;
	int i;

	slash = strchr(name, '/');
	if (!slash || strchr(slash + 1, '/'))
	{
		g_warning(_("Invalid MIME-type '%s'\n"), name);
		return NULL;
	}

	type = g_hash_table_lookup(types, name);
	if (type)
		return type;

	type = g_new(Type, 1);
	type->media = g_strndup(name, slash - name);
	type->subtype = g_strdup(slash + 1);
	g_hash_table_insert(types, g_strdup(name), type);

	type->output = xmlNewDoc("1.0");
	root = xmlNewDocNode(type->output, NULL, "mime-type", NULL);
	ns = xmlNewNs(root, FREE_NS, NULL);
	xmlSetNs(root, ns);
	xmlDocSetRootElement(type->output, root);
	xmlSetNsProp(root, NULL, "type", name);
	xmlAddChild(root, xmlNewDocComment(type->output,
		"Created automatically by update-mime-database. DO NOT EDIT!"));

	for (i = 0; i < G_N_ELEMENTS(media_types); i++)
	{
		if (strcmp(media_types[i], type->media) == 0)
			return type;
	}

	g_warning("Unknown media type in type '%s'\n", name);

	return type;
}

static gboolean match_node(xmlNode *node,
			   const char *namespaceURI,
			   const char *localName)
{
	if (namespaceURI)
		return node->ns &&
			strcmp(node->ns->href, namespaceURI) == 0 &&
			strcmp(node->name, localName) == 0;
	else
		return strcmp(node->name, localName) == 0 && !node->ns;
}

static gboolean validate_magic(xmlNode *parent)
{
	xmlNode *node;

	for (node = parent->xmlChildrenNode; node; node = node->next)
	{
		if (node->type != XML_ELEMENT_NODE)
			continue;
		if (!xmlHasNsProp(node, "offset", NULL))
			return FALSE;
		if (!xmlHasNsProp(node, "type", NULL))
			return FALSE;
		if (!xmlHasNsProp(node, "value", NULL))
			return FALSE;
		if (!validate_magic(node))
			return FALSE;
	}

	return TRUE;
}

/* 'field' was found in the definition of 'type' and has the freedesktop.org
 * namespace. If it's a known field, process it and return TRUE, else
 * return FALSE to add it to the output XML document.
 */
static gboolean process_freedesktop_node(Type *type, xmlNode *field)
{
	if (strcmp(field->name, "glob") == 0)
	{
		gchar *pattern;
		
		pattern = xmlGetNsProp(field, "pattern", NULL);

		g_return_val_if_fail(pattern != NULL, FALSE);

		g_hash_table_insert(globs_hash, g_strdup(pattern), type);
		g_free(pattern);
	}
	else if (strcmp(field->name, "magic") == 0)
	{
		xmlNode *copy;
		gchar *type_name;

		if (validate_magic(field))
		{
			copy = xmlCopyNode(field, 1);
			type_name = g_strconcat(type->media, "/",
						type->subtype, NULL);
			xmlSetNsProp(copy, NULL, "type", type_name);
			g_free(type_name);

			g_ptr_array_add(magic, copy);
		}
		else
			g_print("Skipping invalid magic for type '%s/%s'\n",
				type->media, type->subtype);
	}
	else if (strcmp(field->name, "comment") == 0)
		return FALSE;	/* Copy through */
	else
	{
		g_warning("Unknown freedesktop.org field '%s' "
			  "in type '%s/%s'\n",
			  field->name, type->media, type->subtype);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean has_lang(xmlNode *node, const char *lang)
{
	char *lang2;

	lang2 = xmlGetNsProp(node, "lang", XML_NS);
	if (!lang2)
		return !lang;

	if (strcmp(lang, lang2) == 0)
	{
		g_free(lang2);
		return TRUE;
	}
	return FALSE;
}

/* We're about to add 'new' to the list of fields to be output for the
 * type. Remove any existing nodes which it replaces.
 */
static void remove_old(Type *type, xmlNode *new)
{
	xmlNode *field, *fields;
	gchar *lang;

	if (new->ns == NULL || strcmp(new->ns->href, FREE_NS) != 0)
		return;	/* No idea what we're doing -- leave it in! */

	if (strcmp(new->name, "comment") != 0)
		return;

	lang = xmlGetNsProp(new, "lang", XML_NS);

	fields = xmlDocGetRootElement(type->output);
	for (field = fields->xmlChildrenNode; field; field = field->next)
	{
		if (match_node(field, FREE_NS, "comment") &&
		    has_lang(field, lang))
		{
			xmlUnlinkNode(field);
			xmlFreeNode(field);
			break;
		}
	}

	g_free(lang);
}

static void load_type(xmlNode *node)
{
	char *type_name;
	Type *type;
	xmlNode *field;
	
	type_name = xmlGetNsProp(node, "type", NULL);
	if (!type_name)
	{
		g_warning(_("mime-type element has no 'type' attribute\n"));
		g_free(type_name);
		return;
	}

	type = get_type(type_name);
	g_free(type_name);

	if (!type)
		return;

	for (field = node->xmlChildrenNode; field; field = field->next)
	{
		xmlNode *copy;

		if (field->type != XML_ELEMENT_NODE)
			continue;

		if (field->ns && strcmp(field->ns->href, FREE_NS) == 0)
			if (process_freedesktop_node(type, field))
				continue;

		copy = xmlDocCopyNode(field, type->output, 1);
		
		/* Ugly hack to stop the xmlns= attributes appearing on
		 * every node...
		 */
		if (copy->ns && copy->ns->prefix == NULL &&
			strcmp(copy->ns->href, FREE_NS) == 0)
		{
			if (copy->nsDef)
			{
				/* Still used somewhere... */
				/* xmlFreeNsList(copy->nsDef); */
				/* (this leaks) */
				copy->nsDef = NULL;
			}
		}

		remove_old(type, field);

		xmlAddChild(xmlDocGetRootElement(type->output), copy);
	}
}

#define CHECK_NODE(node, namespaceURI, localName) do {		\
if (!match_node(node, namespaceURI, localName)) { 		\
	g_warning(_("Wrong node namespace or name in %s\n"	\
		    "Expected (%s,%s) but got (%s,%s)\n"),	\
		    filename, namespaceURI, localName,		\
		    node->ns ? (char *) node->ns->href : "none", node->name);\
	goto out;							\
}} while (0);

static void load_source_file(const char *filename)
{
	xmlDoc *doc;
	xmlNode *root, *node;

	doc = xmlParseFile(filename);
	if (!doc)
	{
		g_warning(_("Failed to parse '%s'\n"), filename);
		return;
	}

	root = xmlDocGetRootElement(doc);

	CHECK_NODE(root, FREE_NS, "mime-info");

	for (node = root->xmlChildrenNode; node; node = node->next)
	{
		if (node->type != XML_ELEMENT_NODE)
			continue;

		CHECK_NODE(node, FREE_NS, "mime-type");

		load_type(node);
	}
out:
	xmlFreeDoc(doc);
}

static gint strcmp2(gconstpointer a, gconstpointer b)
{
	const char *aa = *(char **) a;
	const char *bb = *(char **) b;

	return strcmp(aa, bb);
}

static void scan_source_dir(const char *path)
{
	DIR *dir;
	struct dirent *ent;
	char *filename;
	GPtrArray *files;
	int i;
	gboolean have_override = FALSE;

	dir = opendir(path);
	if (!dir)
	{
		perror("scan_source_dir");
		exit(EXIT_FAILURE);
	}

	files = g_ptr_array_new();
	while ((ent = readdir(dir)))
	{
		int l;
		l = strlen(ent->d_name);
		if (l < 4 || strcmp(ent->d_name + l - 4, ".xml") != 0)
			continue;
		if (strcmp(ent->d_name, "Override.xml") == 0)
		{
			have_override = TRUE;
			continue;
		}
		g_ptr_array_add(files, g_strdup(ent->d_name));
	}
	closedir(dir);

	g_ptr_array_sort(files, strcmp2);

	if (have_override)
		g_ptr_array_add(files, g_strdup("Override.xml"));

	for (i = 0; i < files->len; i++)
	{
		gchar *leaf = (gchar *) files->pdata[i];

		filename = g_strconcat(path, "/", leaf, NULL);
		load_source_file(filename);
		g_free(filename);
	}

	for (i = 0; i < files->len; i++)
		g_free(files->pdata[i]);
	g_ptr_array_free(files, TRUE);
}

/* Save doc as XML as filename, 0 on success or -1 on failure */
static int save_xml_file(xmlDocPtr doc, const gchar *filename)
{
#if LIBXML_VERSION > 20400
	if (xmlSaveFormatFileEnc(filename, doc, "utf-8", 1) < 0)
		return 1;
#else
	FILE *out;
	
	out = fopen(filename, "w");
	if (!out)
		return 1;

	xmlDocDump(out, doc);  /* Some versions return void */

	if (fclose(out))
		return 1;
#endif

	return 0;
}

static void write_out_glob(gpointer key, gpointer value, gpointer data)
{
	const gchar *pattern = (gchar *) key;
	Type *type = (Type *) value;
	FILE *stream = (FILE *) data;

	fprintf(stream, "%s/%s:%s\n", type->media, type->subtype, pattern);
}

static void write_out_type(gpointer key, gpointer value, gpointer data)
{
	Type *type = (Type *) value;
	const char *mime_dir = (char *) data;
	char *media, *filename, *new_name;

	media = g_strconcat(mime_dir, "/", type->media, NULL);
	mkdir(media, 0755);

	filename = g_strconcat(media, "/", type->subtype, ".xml.new", NULL);
	g_free(media);
	media = NULL;
	
	if (save_xml_file(type->output, filename) != 0)
		g_warning("Failed to write out '%s'\n", filename);

	new_name = g_strndup(filename, strlen(filename) - 4);
	if (rename(filename, new_name))
		g_warning("Failed to rename %s as %s\n",
				filename, new_name);

	g_free(filename);
	g_free(new_name);
}

static int get_priority(xmlNode *node)
{
	char *prio_string;
	int p;

	prio_string = xmlGetNsProp(node, "priority", NULL);
	if (prio_string)
	{
		p = atoi(prio_string);
		g_free(prio_string);
		g_return_val_if_fail(p >= 0 && p <= 100, 50);
		return p;
	}
	else
		return 50;
}

/* (this is really inefficient) */
static gint cmp_magic(gconstpointer a, gconstpointer b)
{
	xmlNode *aa = *(xmlNode **) a;
	xmlNode *bb = *(xmlNode **) b;
	char *type_a, *type_b;
	int pa, pb;
	int retval;

	g_return_val_if_fail(strcmp(aa->name, "magic") == 0, 0);
	g_return_val_if_fail(strcmp(bb->name, "magic") == 0, 0);

	pa = get_priority(aa);
	pb = get_priority(bb);

	if (pa > pb)
		return -1;
	else if (pa < pb)
		return 1;

	type_a = xmlGetNsProp(aa, "type", NULL);
	type_b = xmlGetNsProp(bb, "type", NULL);
	g_return_val_if_fail(type_a != NULL, 0);
	g_return_val_if_fail(type_b != NULL, 0);

	retval = strcmp(type_a, type_b);

	g_free(type_a);
	g_free(type_b);

	return retval;
}

static void write32(FILE *stream, guint32 n)
{
	guint32 big = GUINT32_TO_BE(n);

	fwrite(&big, sizeof(big), 1, stream);
}

static void write16(FILE *stream, guint32 n)
{
	guint16 big = GUINT16_TO_BE(n);

	g_return_if_fail(n <= 0xffff);

	fwrite(&big, sizeof(big), 1, stream);
}

/* Single hex char to int; -1 if not a hex char.
 * From file(1).
 */
static int hextoint(int c)
{
	if (!isascii((unsigned char) c))
		return -1;
	if (isdigit((unsigned char) c))
		return c - '0';
	if ((c >= 'a')&&(c <= 'f'))
		return c + 10 - 'a';
	if (( c>= 'A')&&(c <= 'F'))
		return c + 10 - 'A';
	return -1;
}

/*
 * Convert a string containing C character escapes.  Stop at an unescaped
 * space or tab.
 * Copy the converted version to "p", returning its length in *slen.
 * Return updated scan pointer as function result.
 * Stolen from file(1) and heavily modified.
 */
static void getstr(const char *s, GString *out)
{
	int	c;
	int	val;

	while ((c = *s++) != '\0') {
		if(c == '\\') {
			switch(c = *s++) {

			case '\0':
				return;

			default:
				g_string_append_c(out, (char) c);
				break;

			case 'n':
				g_string_append_c(out, '\n');
				break;

			case 'r':
				g_string_append_c(out, '\r');
				break;

			case 'b':
				g_string_append_c(out, '\b');
				break;

			case 't':
				g_string_append_c(out, '\t');
				break;

			case 'f':
				g_string_append_c(out, '\f');
				break;

			case 'v':
				g_string_append_c(out, '\v');
				break;

			/* \ and up to 3 octal digits */
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c - '0';
				c = *s++;  /* try for 2 */
				if(c >= '0' && c <= '7') {
					val = (val<<3) | (c - '0');
					c = *s++;  /* try for 3 */
					if(c >= '0' && c <= '7')
						val = (val<<3) | (c-'0');
					else
						--s;
				}
				else
					--s;
				g_string_append_c(out, (char)val);
				break;

			/* \x and up to 2 hex digits */
			case 'x':
				val = 'x';	/* Default if no digits */
				c = hextoint(*s++);	/* Get next char */
				if (c >= 0) {
					val = c;
					c = hextoint(*s++);
					if (c >= 0)
						val = (val << 4) + c;
					else
						--s;
				} else
					--s;
				g_string_append_c(out, (char)val);
				break;
			}
		} else
			g_string_append_c(out, (char)c);
	}
}

static void parse_value(const char *type, const char *in, GString *parsed_value)
{
	char *end;
	long value;

	g_return_if_fail(*in != '\0');

	if (strstr(type, "16"))
	{
		value = strtol(in, &end, 0);
		g_return_if_fail(*end == '\0');
		g_string_append_c(parsed_value, (value >> 8) & 0xff);
		g_string_append_c(parsed_value, value & 0xff);
	}
	else if (strstr(type, "32"))
	{
		value = strtol(in, &end, 0);
		g_return_if_fail(*end == '\0');
		g_string_append_c(parsed_value, (value >> 24) & 0xff);
		g_string_append_c(parsed_value, (value >> 16)& 0xff);
		g_string_append_c(parsed_value, (value >> 8) & 0xff);
		g_string_append_c(parsed_value, value & 0xff);
	}
	else if (strcmp(type, "byte") == 0)
	{
		value = strtol(in, &end, 0);
		g_return_if_fail(*end == '\0');
		g_string_append_c(parsed_value, value & 0xff);
	}
	else if (strcmp(type, "string") == 0)
		getstr(in, parsed_value);
	else
		g_assert_not_reached();
}

static void write_magic_children(FILE *stream, xmlNode *parent, int indent)
{
	GString *parsed_value;
	xmlNode *node;

	parsed_value = g_string_new(NULL);

	for (node = parent->xmlChildrenNode; node; node = node->next)
	{
		char *offset, *mask, *value, *type;
		char *parsed_mask = NULL;
		const char *colon;
		int word_size = 1;
		long range_start;
		int range_length = 1;
		int i;

		if (node->type != XML_ELEMENT_NODE)
			continue;

		for (i = 0; i < indent; i++)
			fputc('>', stream);

		offset = xmlGetNsProp(node, "offset", NULL);
		mask = xmlGetNsProp(node, "mask", NULL);
		value = xmlGetNsProp(node, "value", NULL);
		type = xmlGetNsProp(node, "type", NULL);

		g_return_if_fail(offset != NULL);
		g_return_if_fail(value != NULL);
		g_return_if_fail(type != NULL);

		range_start = atol(offset);
		colon = strchr(offset, ':');
		if (colon)
			range_length = atol(colon + 1) - range_start + 1;

		if (strcmp(type, "host16") == 0)
			word_size = 2;
		else if (strcmp(type, "host32") == 0)
			word_size = 4;
		else if (strcmp(type, "big16") && strcmp(type, "big32") &&
			 strcmp(type, "little16") && strcmp(type, "little32") &&
			 strcmp(type, "string") && strcmp(type, "byte"))
			g_warning("Unknown magic type '%s'\n", type);

		g_string_truncate(parsed_value, 0);
		parse_value(type, value, parsed_value);

		if (mask)
		{
			int i;
			parsed_mask = g_malloc(parsed_value->len);
			for (i = 0; i < parsed_value->len; i++)
				parsed_mask[i] = 0xff;
			/* TODO: Actually read the mask! */
		}

		fputc('=', stream);
		write32(stream, range_start);
		write16(stream, parsed_value->len);
		fwrite(parsed_value->str, parsed_value->len, 1, stream);
		if (parsed_mask)
		{
			fputc('&', stream);
			fwrite(parsed_mask, parsed_value->len, 1, stream);
		}
		if (word_size != 1)
			fprintf(stream, "~%d", word_size);
		if (range_length != 1)
			fprintf(stream, "+%d", range_length);

		fputc('\n', stream);

		g_free(offset);
		g_free(mask);
		g_free(value);
		g_free(type);

		write_magic_children(stream, node, indent + 1);
	}

	g_string_free(parsed_value, TRUE);
}

static void write_magic(FILE *stream, xmlNode *node)
{
	char *type;
	int prio;

	prio = get_priority(node);

	type = xmlGetNsProp(node, "type", NULL);
	g_return_if_fail(type != NULL);
	fprintf(stream, "[%d:%s]\n", prio, type);
	g_free(type);

	write_magic_children(stream, node, 0);
}

static void delete_old_types(const gchar *mime_dir)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS(media_types); i++)
	{
		gchar *media_dir;
		DIR   *dir;
		struct dirent *ent;
		
		media_dir = g_strconcat(mime_dir, "/", media_types[i], NULL);
		dir = opendir(media_dir);
		g_free(media_dir);
		if (!dir)
			continue;

		while ((ent = readdir(dir)))
		{
			char *type_name;
			int l;
			l = strlen(ent->d_name);
			if (l < 4 || strcmp(ent->d_name + l - 4, ".xml") != 0)
				continue;

			type_name = g_strconcat(media_types[i], "/",
						ent->d_name, NULL);
			type_name[strlen(type_name) - 4] = '\0';
			if (!g_hash_table_lookup(types, type_name))
			{
				char *path;
				path = g_strconcat(mime_dir, "/",
						type_name, ".xml", NULL);
				g_print("* Removing old info for type %s\n",
						path);
				unlink(path);
				g_free(path);
			}
			g_free(type_name);
		}
		
		closedir(dir);
	}
}

int main(int argc, char **argv)
{
	const char *mime_dir = NULL;
	char *package_dir = NULL;
	int opt;

	while ((opt = getopt(argc, argv, "hv")) != -1)
	{
		switch (opt)
		{
			case '?':
				usage(argv[0]);
				return EXIT_FAILURE;
			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;
			case 'v':
				fprintf(stderr,
					"update-mime-database (" PACKAGE ") "
					VERSION "\n" COPYING);
				return EXIT_SUCCESS;
			default:
				abort();
		}
	}

	if (optind != argc - 1)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	LIBXML_TEST_VERSION;

	mime_dir = argv[optind];
	package_dir = g_strconcat(mime_dir, "/packages", NULL);

	g_print("***\n* Updating MIME database in %s...\n", mime_dir);
	
	if (access(package_dir, F_OK))
	{
		fprintf(stderr,
			_("Directory '%s' does not exist!\n"), package_dir);
		return EXIT_FAILURE;
	}

	types = g_hash_table_new_full(g_str_hash, g_str_equal,
					g_free, free_type);
	globs_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
					g_free, NULL);
	magic = g_ptr_array_new();

	scan_source_dir(package_dir);
	g_free(package_dir);

	delete_old_types(mime_dir);

	g_hash_table_foreach(types, write_out_type, (gpointer) mime_dir);

	{
		FILE *globs;
		char *globs_path;
		globs_path = g_strconcat(mime_dir, "/globs", NULL);
		globs = fopen(globs_path, "wb");
		if (!globs)
			g_error("Failed to open '%s' for writing\n", globs_path);
		g_free(globs_path);
		fprintf(globs,
			"# This file was automatically generated by the\n"
			"# update-mime-database command. DO NOT EDIT!\n");
		g_hash_table_foreach(globs_hash, write_out_glob, globs);
		fclose(globs);
	}

	{
		FILE *stream;
		char *magic_path;
		int i;
		magic_path = g_strconcat(mime_dir, "/magic", NULL);
		stream = fopen(magic_path, "wb");
		if (!stream)
			g_error("Failed to open '%s' for writing\n", magic_path);
		g_free(magic_path);
		fwrite("MIME-Magic\0\n", 1, 12, stream);

		if (magic->len)
			g_ptr_array_sort(magic, cmp_magic);
		for (i = 0; i < magic->len; i++)
		{
			xmlNode *node = (xmlNode *) magic->pdata[i];

			write_magic(stream, node);
		}
		fclose(stream);
	}

	{
		int i;
		for (i = 0; i < magic->len; i++)
			xmlFreeNode((xmlNode *) magic->pdata[i]);
		g_ptr_array_free(magic, TRUE);
	}

	g_hash_table_destroy(types);
	g_hash_table_destroy(globs_hash);

	g_print("***\n");
	
	return EXIT_SUCCESS;
}
