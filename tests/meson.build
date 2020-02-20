if meson.is_cross_build()
    subdir_done()
endif

if xdgmime_found
    run_target('test-staging',
        command: [
            find_program('test_staging.sh'),
            meson.source_root(),
            meson.build_root(),
            freedesktop_org_xml,
            update_mime_database,
            xdgmime_print_mime_data.path(),
        ]
    )

    test('test-mime',
        find_program('test_mime.sh'),
        args: [
            meson.source_root(),
            meson.build_root(),
            freedesktop_org_xml,
            update_mime_database,
            xdgmime_test_mime_data.path(),
            xdgmime_test_mime.path(),
            test_tree_magic.full_path(),
        ],
    )
endif

run_target('check-generic-icons',
    command: [ find_program('test_generic_icons.sh'), freedesktop_org_xml ],
)

test('No duplicate mime-types',
    find_program('test_duplicate_mime_types.sh'),
    args: freedesktop_org_xml,
)

test('xmllint freedesktop.org.xml',
    xmllint,
    args: [ '--noout', '--valid', freedesktop_org_xml, ],
)

test('Case sensitivity',
    find_program('test-case-sensitivity.sh'),
)

test('update-mime-database',
    update_mime_database,
    args: meson.current_source_dir() / 'mime-db-tests',
)
