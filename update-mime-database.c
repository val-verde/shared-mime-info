#include <config.h>

#define N_(x) x
#define _(x) (x)

#include <string.h>
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

typedef struct _Type Type;
typedef struct _Magic Magic;

struct _Type {
	char *media;
	char *subtype;

	GList	*unknown;	/* xmlNodes for 3rd party extensions */
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
	GList *next;

	g_free(type->media);
	g_free(type->subtype);

	for (next = type->unknown; next; next = next->next)
		xmlFreeNode((xmlNode *) next->data);

	g_list_free(type->unknown);

	g_free(type);
}

static Type *get_type(const char *name)
{
	const char *slash;
	Type *type;

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

	type->unknown = NULL;

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

/* 'field' was found in the definition of 'type' and has the freedesktop.org
 * namespace. If it's a known field, process it and return TRUE, else
 * return FALSE to add it to the unknown fields list (copied to output).
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
		
		copy = xmlCopyNode(field, 1);
		type_name = g_strconcat(type->media, "/", type->subtype, NULL);
		xmlSetNsProp(copy, NULL, "type", type_name);
		g_free(type_name);

		g_ptr_array_add(magic, copy);
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
		lang2 = xmlGetNsProp(node, "lang", NULL); /* (libxml) */
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
	GList *next;
	gchar *lang;

	if (new->ns == NULL || strcmp(new->ns->href, FREE_NS) != 0)
		return;	/* No idea what we're doing -- leave it in! */

	if (strcmp(new->name, "comment") != 0)
		return;

	lang = xmlGetNsProp(new, "lang", XML_NS);
	if (!lang)
		lang = xmlGetNsProp(new, "lang", NULL); /* (libxml) */

	for (next = type->unknown; next; next = next->next)
	{
		xmlNode *node = (xmlNode *) next->data;

		if (match_node(node, FREE_NS, "comment") &&
		    has_lang(node, lang))
		{
			type->unknown = g_list_remove(type->unknown, node);
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

		/* Note that libxml helpfully removes the namespace
		 * from xml:lang when copying. Nice.
		 */
		copy = xmlCopyNode(field, 1);

		remove_old(type, field);

		type->unknown = g_list_append(type->unknown, copy);
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
	const char *type_name = (char *) key;
	char *media, *filename;
	xmlDoc *doc;
	xmlNode *root;
	xmlNs *ns;
	GList *field;

	media = g_strconcat(mime_dir, "/", type->media, NULL);
	mkdir(media, 0755);

	filename = g_strconcat(media, "/", type->subtype, ".xml", NULL);
	g_free(media);
	media = NULL;
	
	doc = xmlNewDoc("1.0");
	root = xmlNewDocNode(doc, NULL, "mime-type", NULL);
	ns = xmlNewNs(root, FREE_NS, NULL);
	xmlSetNs(root, ns);

	xmlSetNsProp(root, NULL, "type", type_name);

	xmlDocSetRootElement(doc, root);

	xmlAddChild(root, xmlNewDocComment(doc,
		"Created automatically by update-mime-database. DO NOT EDIT!"));

	for (field = type->unknown; field; field = field->next)
	{
		xmlNode *copy, *src = (xmlNode *) field->data;

		copy = xmlDocCopyNode(src, doc, 1);

		xmlAddChild(root, copy);

		if (copy->ns && copy->ns->prefix == NULL &&
			strcmp(copy->ns->href, FREE_NS) == 0)
		{
			copy->nsDef = NULL;	/* Yuck! */
		}
	}

	if (save_xml_file(doc, filename) != 0)
		g_warning("Failed to write out '%s'\n", filename);

	xmlFreeDoc(doc);
	g_free(filename);
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
		return 1;
	else if (pa < pb)
		return -1;

	type_a = xmlGetNsProp(aa, "type", NULL);
	type_b = xmlGetNsProp(bb, "type", NULL);
	g_return_val_if_fail(type_a != NULL, 0);
	g_return_val_if_fail(type_b != NULL, 0);

	retval = strcmp(type_a, type_b);

	g_free(type_a);
	g_free(type_b);

	return retval;
}

static void write_magic_children(FILE *stream, xmlNode *parent, int indent)
{
	int i;
	xmlNode *node;

	for (node = parent->xmlChildrenNode; node; node = node->next)
	{
		char *offset, *mask, *value;

		if (node->type != XML_ELEMENT_NODE)
			continue;

		for (i = 0; i < indent; i++)
			fputc('>', stream);

		offset = xmlGetNsProp(node, "offset", NULL);
		mask = xmlGetNsProp(node, "mask", NULL);
		value = xmlGetNsProp(node, "value", NULL);

		if (mask)
			fprintf(stream, "%s\t%s&%s\t%s",
					offset,
					node->name,
					mask,
					value);
		else
			fprintf(stream, "%s\t%s\t%s",
					offset,
					node->name,
					value);
		g_free(offset);

		fputc('\n', stream);
		write_magic_children(stream, node, indent + 1);
	}
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

	mime_dir = argv[optind];
	package_dir = g_strconcat(mime_dir, "/packages", NULL);
	
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

	g_hash_table_foreach(types, write_out_type, (gpointer) mime_dir);

	{
		FILE *globs;
		char *globs_path;
		globs_path = g_strconcat(mime_dir, "/globs", NULL);
		globs = fopen(globs_path, "wb");
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
		int  i;
		magic_path = g_strconcat(mime_dir, "/magic", NULL);
		stream = fopen(magic_path, "wb");
		g_free(magic_path);
		fprintf(stream,
			"# This file was automatically generated by the\n"
			"# update-mime-database command. DO NOT EDIT!\n");
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
	
	return EXIT_SUCCESS;
}
