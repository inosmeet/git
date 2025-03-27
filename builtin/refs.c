#define USE_THE_REPOSITORY_VARIABLE
#include "builtin.h"
#include "config.h"
#include "fsck.h"
#include "parse-options.h"
#include "refs.h"
#include "strbuf.h"
#include "worktree.h"
#include "object-name.h"

#define REFS_MIGRATE_USAGE \
	N_("git refs migrate --ref-format=<format> [--no-reflog] [--dry-run]")

#define REFS_VERIFY_USAGE \
	N_("git refs verify [--strict] [--verbose]")

#define REFS_LIST_USAGE \
	N_("git refs list [--head] [--branches] [--tag]")

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

	worktrees = get_worktrees();
	for (size_t i = 0; worktrees[i]; i++)
		ret |= refs_fsck(get_worktree_ref_store(worktrees[i]),
				 &fsck_refs_options, worktrees[i]);

	fsck_options_clear(&fsck_refs_options);
	free_worktrees(worktrees);
	return ret;
}

struct patterns_options {
	int show_head;
	int branches_only;
	int tags_only;
};

static int show_ref(const char *refname, const char *referent UNUSED, const struct object_id *oid,
		    int flag UNUSED, void *cbdata UNUSED)
{
	const char *hex;

	hex = repo_find_unique_abbrev(the_repository, oid, 0);
	printf("%s %s\n", hex, refname);

	return 0;
}

static int cmd_refs_list(int argc, const char **argv, const char *prefix,
			 struct repository *repo UNUSED)
{
	struct patterns_options patterns_opts = {0};
	const char * const list_usage[] = {
		REFS_LIST_USAGE,
		NULL,
	};
	struct option options[] = {
		OPT_BOOL(0, "head", &patterns_opts.show_head,
		  N_("show the HEAD reference, even if it would be filtered out")),
		OPT_BOOL(0, "tags", &patterns_opts.tags_only, N_("only show tags (can be combined with --branches)")),
		OPT_BOOL(0, "branches", &patterns_opts.branches_only, N_("only show branches (can be combined with --tags)")),
		OPT_HIDDEN_BOOL(0, "heads", &patterns_opts.branches_only,
				N_("deprecated synonym for --branches")),
		OPT_END(),
	};

	argc = parse_options(argc, argv, prefix, options, list_usage, 0);
	if (argc)
		usage(_("'git refs list' takes no arguments"));

	if (patterns_opts.show_head)
		refs_head_ref(get_main_ref_store(the_repository), show_ref,
			      &patterns_opts);
	if (patterns_opts.tags_only || patterns_opts.branches_only) {
		if (patterns_opts.branches_only)
			refs_for_each_fullref_in(get_main_ref_store(the_repository),
					"refs/heads/", NULL, show_ref, &patterns_opts);

		if (patterns_opts.tags_only)
			refs_for_each_fullref_in(get_main_ref_store(the_repository),
					 "refs/tags/", NULL, show_ref, &patterns_opts);
	}
	else refs_for_each_ref(get_main_ref_store(the_repository), show_ref, &patterns_opts);

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
