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

#define XML_NS "http://www.w3.org/XML/1998/namespace"
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

struct _Type {
	char *media;
	char *subtype;

	GList	*unknown;	/* xmlNodes for 3rd party extensions */
};
	     
/* Maps MIME type names to Types */
static GHashTable *types = NULL;
	     
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

		copy = xmlCopyNode(field, 1);

		type->unknown = g_list_append(type->unknown, copy);
	}
}

#define CHECK_NODE(node, namespaceURI, localName) do { \
if (strcmp(node->ns->href, namespaceURI) || strcmp(node->name, localName)) { \
	g_warning(_("Wrong node namespace or name in %s\n"), filename);	     \
	goto out;							\
} } while (0);

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

	g_print("[ loaded %s ]\n", filename);

out:
	xmlFreeDoc(doc);
}

static void scan_source_dir(const char *path)
{
	DIR *dir;
	struct dirent *ent;
	char *filename;

	dir = opendir(path);
	if (!dir)
	{
		perror("scan_source_dir");
		exit(EXIT_FAILURE);
	}

	while ((ent = readdir(dir)))
	{
		int l;

		/* Only process files with names ending in '.xml' */
		l = strlen(ent->d_name);
		if (l < 4 || strcmp(ent->d_name + l - 4, ".xml") != 0)
			continue;
		
		filename = g_strconcat(path, "/", ent->d_name, NULL);
		load_source_file(filename);
		g_free(filename);
	}

	closedir(dir);
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
	
	g_print("[ write %s ]\n", filename);

	doc = xmlNewDoc("1.0");
	root = xmlNewDocNode(doc, NULL, "mime-type", NULL);
	ns = xmlNewNs(root, FREE_NS, NULL);
	xmlSetNs(root, ns);

	xmlSetNsProp(root, NULL, "type", type_name);

	xmlDocSetRootElement(doc, root);

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

	types = g_hash_table_new_full(NULL, NULL, g_free, free_type);

	scan_source_dir(package_dir);
	g_free(package_dir);

	g_hash_table_foreach(types, write_out_type, (gpointer) mime_dir);

	g_hash_table_destroy(types);
	
	return EXIT_SUCCESS;
}
