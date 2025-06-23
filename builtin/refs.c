#define USE_THE_REPOSITORY_VARIABLE
#include "builtin.h"
#include "config.h"
#include "fsck.h"
#include "parse-options.h"
#include "ref-filter.h"
#include "refs.h"
#include "strbuf.h"
#include "worktree.h"

#define REFS_MIGRATE_USAGE \
	N_("git refs migrate --ref-format=<format> [--no-reflog] [--dry-run]")

#define REFS_VERIFY_USAGE \
	N_("git refs verify [--strict] [--verbose]")

#define REFS_LIST_USAGE \
	N_("git refs list [--count=<count>] [--shell|--perl|--python|--tcl]\n" \
	   "              [(--sort=<key>)...] [--format=<format>]\n" \
	   "              [--include-root-refs] [ --stdin | <pattern>... ]\n" \
	   "              [--points-at=<object>]\n" \
	   "              [--merged[=<object>]] [--no-merged[=<object>]]\n" \
	   "              [--contains[=<object>]] [--no-contains[=<object>]]\n" \
	   "              [--exclude=<pattern> ...]")

static int cmd_refs_migrate(int argc, const char **argv, const char *prefix,
			    struct repository *repo UNUSED)
{
	const char * const migrate_usage[] = {
		REFS_MIGRATE_USAGE,
		NULL,
	};
	const char *format_str = NULL;
	enum ref_storage_format format;
	unsigned int flags = 0;
	struct option options[] = {
		OPT_STRING_F(0, "ref-format", &format_str, N_("format"),
			N_("specify the reference format to convert to"),
			PARSE_OPT_NONEG),
		OPT_BIT(0, "dry-run", &flags,
			N_("perform a non-destructive dry-run"),
			REPO_MIGRATE_REF_STORAGE_FORMAT_DRYRUN),
		OPT_BIT(0, "no-reflog", &flags,
			N_("drop reflogs entirely during the migration"),
			REPO_MIGRATE_REF_STORAGE_FORMAT_SKIP_REFLOG),
		OPT_END(),
	};
	struct strbuf errbuf = STRBUF_INIT;
	int err;

	argc = parse_options(argc, argv, prefix, options, migrate_usage, 0);
	if (argc)
		usage(_("too many arguments"));
	if (!format_str)
		usage(_("missing --ref-format=<format>"));

	format = ref_storage_format_by_name(format_str);
	if (format == REF_STORAGE_FORMAT_UNKNOWN) {
		err = error(_("unknown ref storage format '%s'"), format_str);
		goto out;
	}

	if (the_repository->ref_storage_format == format) {
		err = error(_("repository already uses '%s' format"),
			    ref_storage_format_to_name(format));
		goto out;
	}

	if (repo_migrate_ref_storage_format(the_repository, format, flags, &errbuf) < 0) {
		err = error("%s", errbuf.buf);
		goto out;
	}

	err = 0;

out:
	strbuf_release(&errbuf);
	return err;
}

static int cmd_refs_verify(int argc, const char **argv, const char *prefix,
			   struct repository *repo UNUSED)
{
	struct fsck_options fsck_refs_options = FSCK_REFS_OPTIONS_DEFAULT;
	struct worktree **worktrees;
	const char * const verify_usage[] = {
		REFS_VERIFY_USAGE,
		NULL,
	};
	struct option options[] = {
		OPT_BOOL(0, "verbose", &fsck_refs_options.verbose, N_("be verbose")),
		OPT_BOOL(0, "strict", &fsck_refs_options.strict, N_("enable strict checking")),
		OPT_END(),
	};
	int ret = 0;

	argc = parse_options(argc, argv, prefix, options, verify_usage, 0);
	if (argc)
		usage(_("'git refs verify' takes no arguments"));

	git_config(git_fsck_config, &fsck_refs_options);
	prepare_repo_settings(the_repository);

	worktrees = get_worktrees_without_reading_head();
	for (size_t i = 0; worktrees[i]; i++)
		ret |= refs_fsck(get_worktree_ref_store(worktrees[i]),
				 &fsck_refs_options, worktrees[i]);

	fsck_options_clear(&fsck_refs_options);
	free_worktrees(worktrees);
	return ret;
}

static int cmd_refs_list(int argc, const char **argv, const char *prefix,
			 struct repository *repo)
{
	struct ref_sorting *sorting;
	struct string_list sorting_options = STRING_LIST_INIT_DUP;
	int icase = 0, include_root_refs = 0, from_stdin = 0;
	struct ref_filter filter = REF_FILTER_INIT;
	struct ref_format format = REF_FORMAT_INIT;
	unsigned int flags = FILTER_REFS_REGULAR;
	struct strvec vec = STRVEC_INIT;
	const char * const list_usage[] = {
		REFS_LIST_USAGE,
		NULL
	};

	struct option opts[] = {
		OPT_BIT('s', "shell", &format.quote_style,
			N_("quote placeholders suitably for shells"), QUOTE_SHELL),
		OPT_BIT('p', "perl",  &format.quote_style,
			N_("quote placeholders suitably for perl"), QUOTE_PERL),
		OPT_BIT(0 , "python", &format.quote_style,
			N_("quote placeholders suitably for python"), QUOTE_PYTHON),
		OPT_BIT(0 , "tcl",  &format.quote_style,
			N_("quote placeholders suitably for Tcl"), QUOTE_TCL),
		OPT_BOOL(0, "omit-empty",  &format.array_opts.omit_empty,
			N_("do not output a newline after empty formatted refs")),

		OPT_GROUP(""),
		OPT_INTEGER(0, "count", &format.array_opts.max_count, N_("show only <n> matched refs")),
		OPT_STRING(0, "format", &format.format, N_("format"), N_("format to use for the output")),
		OPT__COLOR(&format.use_color, N_("respect format colors")),
		OPT_REF_FILTER_EXCLUDE(&filter),
		OPT_REF_SORT(&sorting_options),
		OPT_CALLBACK(0, "points-at", &filter.points_at,
			     N_("object"), N_("print only refs which points at the given object"),
			     parse_opt_object_name),
		OPT_MERGED(&filter, N_("print only refs that are merged")),
		OPT_NO_MERGED(&filter, N_("print only refs that are not merged")),
		OPT_CONTAINS(&filter.with_commit, N_("print only refs which contain the commit")),
		OPT_NO_CONTAINS(&filter.no_commit, N_("print only refs which don't contain the commit")),
		OPT_BOOL(0, "ignore-case", &icase, N_("sorting and filtering are case insensitive")),
		OPT_BOOL(0, "stdin", &from_stdin, N_("read reference patterns from stdin")),
		OPT_BOOL(0, "include-root-refs", &include_root_refs, N_("also include HEAD ref and pseudorefs")),
		OPT_END(),
	};

	format.format = "%(objectname) %(objecttype)\t%(refname)";

	repo_config(repo, git_default_config, NULL);

	/* Default to sorting by refname unless overridden by --sort= */
	string_list_append(&sorting_options, "refname");

	parse_options(argc, argv, prefix, opts, list_usage, 0);
	if (format.array_opts.max_count < 0) {
		error("invalid --count value: `%d'", format.array_opts.max_count);
		usage_with_options(list_usage, opts);
	}
	if (HAS_MULTI_BITS(format.quote_style)) {
		error("more than one quoting style?");
		usage_with_options(list_usage, opts);
	}
	if (verify_ref_format(&format))
		usage_with_options(list_usage, opts);

	sorting = ref_sorting_options(&sorting_options);
	ref_sorting_set_sort_flags_all(sorting, REF_SORTING_ICASE, icase);
	filter.ignore_case = icase;

	if (from_stdin) {
		struct strbuf line = STRBUF_INIT;

		if (argv[0])
			die(_("unknown arguments supplied with --stdin"));

		while (strbuf_getline(&line, stdin) != EOF)
			strvec_push(&vec, line.buf);

		strbuf_release(&line);

		/* vec.v is NULL-terminated, just like 'argv'. */
		filter.name_patterns = vec.v;
	} else {
		filter.name_patterns = argv;
	}

	if (include_root_refs)
		flags |= FILTER_REFS_ROOT_REFS | FILTER_REFS_DETACHED_HEAD;

	filter.match_as_path = 1;
	filter_and_format_refs(&filter, flags, sorting, &format);

	ref_filter_clear(&filter);
	ref_sorting_release(sorting);
	strvec_clear(&vec);
	return 0;
}

int cmd_refs(int argc,
	     const char **argv,
	     const char *prefix,
	     struct repository *repo)
{
	const char * const refs_usage[] = {
		REFS_MIGRATE_USAGE,
		REFS_VERIFY_USAGE,
		REFS_LIST_USAGE,
		NULL,
	};
	parse_opt_subcommand_fn *fn = NULL;
	struct option opts[] = {
		OPT_SUBCOMMAND("migrate", &fn, cmd_refs_migrate),
		OPT_SUBCOMMAND("verify", &fn, cmd_refs_verify),
		OPT_SUBCOMMAND("list", &fn, cmd_refs_list),
		OPT_END(),
	};

	argc = parse_options(argc, argv, prefix, opts, refs_usage, 0);
	return fn(argc, argv, prefix, repo);
}
