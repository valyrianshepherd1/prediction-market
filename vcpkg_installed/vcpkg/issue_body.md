Package: libpq[core,lz4,openssl,zlib]:x64-mingw-dynamic@16.9#3

**Host Environment**

- Host: x64-windows
- Compiler: GNU 13.2.0
- CMake Version: 3.31.10
-    vcpkg-tool version: 2025-12-16-44bb3ce006467fc13ba37ca099f64077b8bbf84d
    vcpkg-scripts version: a34a3811fc 2026-02-24 (2 weeks ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
-- Using cached postgresql-16.9.tar.bz2
-- Extracting source C:/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/downloads/postgresql-16.9.tar.bz2
-- Applying patch unix/installdirs.patch
-- Applying patch unix/fix-configure.patch
-- Applying patch unix/single-linkage.patch
-- Applying patch unix/no-server-tools.patch
-- Applying patch unix/mingw-install.patch
-- Applying patch unix/mingw-static-importlib-fix.patch
-- Applying patch unix/python.patch
-- Applying patch windows/macro-def.patch
-- Applying patch windows/win_bison_flex.patch
-- Applying patch windows/msbuild.patch
-- Applying patch windows/spin_delay.patch
-- Applying patch windows/tcl-9.0-alpha.patch
-- Applying patch android/unversioned_so.patch
-- Using source at C:/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/src/tgresql-16-09acf38b01.clean
-- Getting CMake variables for x64-mingw-dynamic
-- Loading CMake variables from C:/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/cmake-get-vars_C_CXX-x64-mingw-dynamic.cmake.log
-- Getting CMake variables for x64-mingw-dynamic
-- Using cached tzcode-2025b-1-x86_64.pkg.tar.zst
-- Using cached msys2-autoconf-wrapper-20250528-1-any.pkg.tar.zst
-- Using cached msys2-automake-wrapper-20250528-1-any.pkg.tar.zst
-- Using cached msys2-binutils-2.45.1-1-x86_64.pkg.tar.zst
-- Using cached msys2-libtool-2.5.4-4-x86_64.pkg.tar.zst
-- Using cached msys2-make-4.4.1-2-x86_64.pkg.tar.zst
-- Using cached msys2-pkgconf-2.5.1-1-x86_64.pkg.tar.zst
-- Using cached msys2-which-2.23-4-x86_64.pkg.tar.zst
-- Using cached msys2-autoconf-archive-2024.10.16-1-any.pkg.tar.zst
-- Using cached msys2-bash-5.3.009-1-x86_64.pkg.tar.zst
-- Using cached msys2-coreutils-8.32-5-x86_64.pkg.tar.zst
-- Using cached msys2-file-5.46-2-x86_64.pkg.tar.zst
-- Using cached msys2-gawk-5.3.2-1-x86_64.pkg.tar.zst
-- Using cached msys2-grep-1~3.0-7-x86_64.pkg.tar.zst
-- Using cached msys2-gzip-1.14-1-x86_64.pkg.tar.zst
-- Using cached msys2-diffutils-3.12-1-x86_64.pkg.tar.zst
-- Using cached msys2-sed-4.9-1-x86_64.pkg.tar.zst
-- Using cached msys2-msys2-runtime-3.6.5-1-x86_64.pkg.tar.zst
-- Using cached msys2-autoconf2.72-2.72-3-any.pkg.tar.zst
-- Using cached msys2-automake1.16-1.16.5-1-any.pkg.tar.zst
-- Using cached msys2-automake1.17-1.17-1-any.pkg.tar.zst
-- Using cached msys2-automake1.18-1.18.1-1-any.pkg.tar.zst
-- Using cached msys2-libiconv-1.18-1-x86_64.pkg.tar.zst
-- Using cached msys2-libintl-0.22.5-1-x86_64.pkg.tar.zst
-- Using cached msys2-zlib-1.3.1-1-x86_64.pkg.tar.zst
-- Using cached msys2-findutils-4.10.0-2-x86_64.pkg.tar.zst
-- Using cached msys2-tar-1.35-3-x86_64.pkg.tar.zst
-- Using cached msys2-gmp-6.3.0-2-x86_64.pkg.tar.zst
-- Using cached msys2-gcc-libs-15.2.0-1-x86_64.pkg.tar.zst
-- Using cached msys2-libbz2-1.0.8-4-x86_64.pkg.tar.zst
-- Using cached msys2-liblzma-5.8.2-1-x86_64.pkg.tar.zst
-- Using cached msys2-libzstd-1.5.7-1-x86_64.pkg.tar.zst
-- Using cached msys2-libreadline-8.3.003-1-x86_64.pkg.tar.zst
-- Using cached msys2-mpfr-4.2.2-1-x86_64.pkg.tar.zst
-- Using cached msys2-libpcre-8.45-5-x86_64.pkg.tar.zst
-- Using cached msys2-m4-1.4.19-2-x86_64.pkg.tar.zst
-- Using cached msys2-perl-5.40.3-1-x86_64.pkg.tar.zst
-- Using cached msys2-ncurses-6.5.20240831-2-x86_64.pkg.tar.zst
-- Using cached msys2-libxcrypt-4.5.2-1-x86_64.pkg.tar.zst
-- Using msys root at C:/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/downloads/tools/msys2/f3628c0f6cbb486c
-- Generating configure for x64-mingw-dynamic
-- Finished generating configure for x64-mingw-dynamic
-- Using cached msys2-mingw-w64-x86_64-pkgconf-1~2.5.1-1-any.pkg.tar.zst
-- Using cached msys2-msys2-runtime-3.6.5-1-x86_64.pkg.tar.zst
-- Using msys root at C:/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/downloads/tools/msys2/3e71d1f8e22ab23f
-- Configuring x64-mingw-dynamic-dbg
-- Configuring x64-mingw-dynamic-rel
-- Building x64-mingw-dynamic-dbg
CMake Error at scripts/cmake/vcpkg_execute_build_process.cmake:134 (message):
    Command failed: C:/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/downloads/tools/msys2/f3628c0f6cbb486c/usr/bin/make.exe -j 5 --trace -f Makefile all
    Working Directory: C:/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/
    See logs for more information:
      C:\Users\aleks\CLionProjects\prediction-market\extern\vcpkg\buildtrees\libpq\build-x64-mingw-dynamic-dbg-out.log
      C:\Users\aleks\CLionProjects\prediction-market\extern\vcpkg\buildtrees\libpq\build-x64-mingw-dynamic-dbg-err.log

Call Stack (most recent call first):
  scripts/cmake/vcpkg_build_make.cmake:136 (vcpkg_execute_build_process)
  scripts/cmake/vcpkg_install_make.cmake:2 (vcpkg_build_make)
  ports/libpq/portfile.cmake:135 (vcpkg_install_make)
  scripts/ports.cmake:206 (include)



```

<details><summary>C:\Users\aleks\CLionProjects\prediction-market\extern\vcpkg\buildtrees\libpq\build-x64-mingw-dynamic-dbg-err.log</summary>

```
config_info.c: In function 'get_configdata':
config_info.c:127:55: error: incomplete universal character name \U
  127 |         configdata[i].setting = pstrdup(CONFIGURE_ARGS);
      |                                                       ^
config_info.c:127:55: warning: unknown escape sequence: '\C'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: warning: unknown escape sequence: '\l'
config_info.c:127:55: warning: unknown escape sequence: '\d'
config_info.c:127:55: warning: unknown escape sequence: '\l'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: error: incomplete universal character name \U
config_info.c:127:55: warning: unknown escape sequence: '\C'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: warning: unknown escape sequence: '\l'
config_info.c:127:55: warning: unknown escape sequence: '\s'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: error: incomplete universal character name \U
config_info.c:127:55: warning: unknown escape sequence: '\C'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: warning: unknown escape sequence: '\d'
config_info.c:127:55: warning: unknown escape sequence: '\l'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: error: incomplete universal character name \U
config_info.c:127:55: warning: unknown escape sequence: '\C'
config_info.c:127:55: warning: unknown escape sequence: '\p'
config_info.c:127:55: warning: unknown escape sequence: '\s'
config_info.c:127:55: warning: unknown escape sequence: '\p'
make[1]: *** [<builtin>: config_info.o] Error 1
make[1]: *** Waiting for unfinished jobs....
make: *** [Makefile:19: all] Error 2
```
</details>

<details><summary>C:\Users\aleks\CLionProjects\prediction-market\extern\vcpkg\buildtrees\libpq\build-x64-mingw-dynamic-dbg-out.log</summary>

```
src/Makefile.global:386: update target 'submake-generated-headers' due to: target is .PHONY
/usr/bin/make -C ./src/backend generated-headers
make[1]: Entering directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend'
Makefile:138: update target 'submake-catalog-headers' due to: target is .PHONY
/usr/bin/make -C catalog distprep generated-header-symlinks
Makefile:142: update target 'submake-nodes-headers' due to: target is .PHONY
/usr/bin/make -C nodes distprep generated-header-symlinks
Makefile:146: update target 'submake-utils-headers' due to: target is .PHONY
/usr/bin/make -C utils distprep generated-header-symlinks
Makefile:166: update target '../../src/include/storage/lwlocknames.h' due to: target does not exist
prereqdir=`cd 'storage/lmgr/' >/dev/null && pwd` && \
  cd '../../src/include/storage/' && rm -f lwlocknames.h && \
  cp -pR "$prereqdir/lwlocknames.h" .
make[2]: Entering directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/nodes'
make[2]: Nothing to be done for 'distprep'.
Makefile:89: update target '../../../src/include/nodes/header-stamp' due to: target does not exist
prereqdir=`cd './' >/dev/null && pwd` && \
cd '../../../src/include/nodes/' && for file in nodetags.h; do \
  rm -f $file && cp -pR "$prereqdir/$file" . ; \
done
make[2]: Entering directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/utils'
make[2]: Nothing to be done for 'distprep'.
Makefile:46: update target 'submake-adt-headers' due to: target is .PHONY
/usr/bin/make -C adt jsonpath_gram.h
make[2]: Entering directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/catalog'
Makefile:109: update target 'bki-stamp' due to: ../../../configure.ac
'/usr/bin/perl' genbki.pl --include-path=../../../src/include/ \
	--set-version=16 ../../../src/include/catalog/pg_proc.h ../../../src/include/catalog/pg_type.h ../../../src/include/catalog/pg_attribute.h ../../../src/include/catalog/pg_class.h ../../../src/include/catalog/pg_attrdef.h ../../../src/include/catalog/pg_constraint.h ../../../src/include/catalog/pg_inherits.h ../../../src/include/catalog/pg_index.h ../../../src/include/catalog/pg_operator.h ../../../src/include/catalog/pg_opfamily.h ../../../src/include/catalog/pg_opclass.h ../../../src/include/catalog/pg_am.h ../../../src/include/catalog/pg_amop.h ../../../src/include/catalog/pg_amproc.h ../../../src/include/catalog/pg_language.h ../../../src/include/catalog/pg_largeobject_metadata.h ../../../src/include/catalog/pg_largeobject.h ../../../src/include/catalog/pg_aggregate.h ../../../src/include/catalog/pg_statistic.h ../../../src/include/catalog/pg_statistic_ext.h ../../../src/include/catalog/pg_statistic_ext_data.h ../../../src/include/catalog/pg_rewrite.h ../../../src/include/catalog/pg_trigger.h ../../../src/include/catalog/pg_event_trigger.h ../../../src/include/catalog/pg_description.h ../../../src/include/catalog/pg_cast.h ../../../src/include/catalog/pg_enum.h ../../../src/include/catalog/pg_namespace.h ../../../src/include/catalog/pg_conversion.h ../../../src/include/catalog/pg_depend.h ../../../src/include/catalog/pg_database.h ../../../src/include/catalog/pg_db_role_setting.h ../../../src/include/catalog/pg_tablespace.h ../../../src/include/catalog/pg_authid.h ../../../src/include/catalog/pg_auth_members.h ../../../src/include/catalog/pg_shdepend.h ../../../src/include/catalog/pg_shdescription.h ../../../src/include/catalog/pg_ts_config.h ../../../src/include/catalog/pg_ts_config_map.h ../../../src/include/catalog/pg_ts_dict.h ../../../src/include/catalog/pg_ts_parser.h ../../../src/include/catalog/pg_ts_template.h ../../../src/include/catalog/pg_extension.h ../../../src/include/catalog/pg_foreign_data_wrapper.h ../../../src/include/catalog/pg_foreign_server.h ../../../src/include/catalog/pg_user_mapping.h ../../../src/include/catalog/pg_foreign_table.h ../../../src/include/catalog/pg_policy.h ../../../src/include/catalog/pg_replication_origin.h ../../../src/include/catalog/pg_default_acl.h ../../../src/include/catalog/pg_init_privs.h ../../../src/include/catalog/pg_seclabel.h ../../../src/include/catalog/pg_shseclabel.h ../../../src/include/catalog/pg_collation.h ../../../src/include/catalog/pg_parameter_acl.h ../../../src/include/catalog/pg_partitioned_table.h ../../../src/include/catalog/pg_range.h ../../../src/include/catalog/pg_transform.h ../../../src/include/catalog/pg_sequence.h ../../../src/include/catalog/pg_publication.h ../../../src/include/catalog/pg_publication_namespace.h ../../../src/include/catalog/pg_publication_rel.h ../../../src/include/catalog/pg_subscription.h ../../../src/include/catalog/pg_subscription_rel.h
Makefile:77: update target '../../../src/include/utils/header-stamp' due to: target does not exist
prereqdir=`cd './' >/dev/null && pwd` && \
cd '../../../src/include/utils/' && for file in fmgroids.h fmgrprotos.h errcodes.h; do \
  rm -f $file && cp -pR "$prereqdir/$file" . ; \
done
make[3]: Entering directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/utils/adt'
make[3]: 'jsonpath_gram.h' is up to date.
make[3]: Leaving directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/utils/adt'
Makefile:69: update target 'probes.h' due to: target does not exist
sed -f Gen_dummy_probes.sed probes.d >probes.h
touch ../../../src/include/nodes/header-stamp
Makefile:85: update target '../../../src/include/utils/probes.h' due to: target does not exist
cd '../../../src/include/utils/' && rm -f probes.h && \
    cp -pR "../../../src/backend/utils/probes.h" .
make[2]: Leaving directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/nodes'
touch ../../../src/include/utils/header-stamp
make[2]: Leaving directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/utils'
touch bki-stamp
Makefile:118: update target '../../../src/include/catalog/header-stamp' due to: target does not exist
prereqdir=`cd './' >/dev/null && pwd` && \
cd '../../../src/include/catalog/' && for file in pg_proc_d.h pg_type_d.h pg_attribute_d.h pg_class_d.h pg_attrdef_d.h pg_constraint_d.h pg_inherits_d.h pg_index_d.h pg_operator_d.h pg_opfamily_d.h pg_opclass_d.h pg_am_d.h pg_amop_d.h pg_amproc_d.h pg_language_d.h pg_largeobject_metadata_d.h pg_largeobject_d.h pg_aggregate_d.h pg_statistic_d.h pg_statistic_ext_d.h pg_statistic_ext_data_d.h pg_rewrite_d.h pg_trigger_d.h pg_event_trigger_d.h pg_description_d.h pg_cast_d.h pg_enum_d.h pg_namespace_d.h pg_conversion_d.h pg_depend_d.h pg_database_d.h pg_db_role_setting_d.h pg_tablespace_d.h pg_authid_d.h pg_auth_members_d.h pg_shdepend_d.h pg_shdescription_d.h pg_ts_config_d.h pg_ts_config_map_d.h pg_ts_dict_d.h pg_ts_parser_d.h pg_ts_template_d.h pg_extension_d.h pg_foreign_data_wrapper_d.h pg_foreign_server_d.h pg_user_mapping_d.h pg_foreign_table_d.h pg_policy_d.h pg_replication_origin_d.h pg_default_acl_d.h pg_init_privs_d.h pg_seclabel_d.h pg_shseclabel_d.h pg_collation_d.h pg_parameter_acl_d.h pg_partitioned_table_d.h pg_range_d.h pg_transform_d.h pg_sequence_d.h pg_publication_d.h pg_publication_namespace_d.h pg_publication_rel_d.h pg_subscription_d.h pg_subscription_rel_d.h schemapg.h system_fk_info.h; do \
  rm -f $file && cp -pR "$prereqdir/$file" . ; \
done
touch ../../../src/include/catalog/header-stamp
make[2]: Leaving directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend/catalog'
make[1]: Leaving directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/backend'
Makefile:18: update target 'all' due to: target is .PHONY
/usr/bin/make -C src/include
make[1]: Entering directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/include'
make[1]: Nothing to be done for 'all'.
make[1]: Leaving directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/include'
/usr/bin/make -C src/common
make[1]: Entering directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/common'
<builtin>: update target 'archive.o' due to: target does not exist
C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g -DFRONTEND -I. -I../../src/common -I../../src/include  -I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include  "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL -DVAL_CC="\"C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe\"" -DVAL_CPPFLAGS="\"-I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL\"" -DVAL_CFLAGS="\"-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g\"" -DVAL_CFLAGS_SL="\"\"" -DVAL_LDFLAGS="\"-LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -Wl,--allow-multiple-definition -Wl,--disable-auto-import -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib\"" -DVAL_LDFLAGS_EX="\"\"" -DVAL_LDFLAGS_SL="\"\"" -DVAL_LIBS="\"-lpgcommon -lpgport -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -llz4d -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lssl -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lcrypto -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lzlibd -lpthread -lws2_32 -ldl -lm -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -Wl,-Bstatic,-luuid,-Bdynamic -lcomdlg32 -ladvapi32 -lws2_32\""  -c -o archive.o archive.c
<builtin>: update target 'base64.o' due to: target does not exist
C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g -DFRONTEND -I. -I../../src/common -I../../src/include  -I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include  "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL -DVAL_CC="\"C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe\"" -DVAL_CPPFLAGS="\"-I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL\"" -DVAL_CFLAGS="\"-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g\"" -DVAL_CFLAGS_SL="\"\"" -DVAL_LDFLAGS="\"-LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -Wl,--allow-multiple-definition -Wl,--disable-auto-import -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib\"" -DVAL_LDFLAGS_EX="\"\"" -DVAL_LDFLAGS_SL="\"\"" -DVAL_LIBS="\"-lpgcommon -lpgport -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -llz4d -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lssl -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lcrypto -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lzlibd -lpthread -lws2_32 -ldl -lm -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -Wl,-Bstatic,-luuid,-Bdynamic -lcomdlg32 -ladvapi32 -lws2_32\""  -c -o base64.o base64.c
<builtin>: update target 'checksum_helper.o' due to: target does not exist
C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g -DFRONTEND -I. -I../../src/common -I../../src/include  -I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include  "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL -DVAL_CC="\"C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe\"" -DVAL_CPPFLAGS="\"-I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL\"" -DVAL_CFLAGS="\"-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g\"" -DVAL_CFLAGS_SL="\"\"" -DVAL_LDFLAGS="\"-LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -Wl,--allow-multiple-definition -Wl,--disable-auto-import -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib\"" -DVAL_LDFLAGS_EX="\"\"" -DVAL_LDFLAGS_SL="\"\"" -DVAL_LIBS="\"-lpgcommon -lpgport -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -llz4d -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lssl -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lcrypto -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lzlibd -lpthread -lws2_32 -ldl -lm -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -Wl,-Bstatic,-luuid,-Bdynamic -lcomdlg32 -ladvapi32 -lws2_32\""  -c -o checksum_helper.o checksum_helper.c
<builtin>: update target 'compression.o' due to: target does not exist
C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g -DFRONTEND -I. -I../../src/common -I../../src/include  -I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include  "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL -DVAL_CC="\"C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe\"" -DVAL_CPPFLAGS="\"-I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL\"" -DVAL_CFLAGS="\"-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g\"" -DVAL_CFLAGS_SL="\"\"" -DVAL_LDFLAGS="\"-LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -Wl,--allow-multiple-definition -Wl,--disable-auto-import -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib\"" -DVAL_LDFLAGS_EX="\"\"" -DVAL_LDFLAGS_SL="\"\"" -DVAL_LIBS="\"-lpgcommon -lpgport -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -llz4d -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lssl -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lcrypto -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lzlibd -lpthread -lws2_32 -ldl -lm -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -Wl,-Bstatic,-luuid,-Bdynamic -lcomdlg32 -ladvapi32 -lws2_32\""  -c -o compression.o compression.c
<builtin>: update target 'config_info.o' due to: target does not exist
C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g -DFRONTEND -I. -I../../src/common -I../../src/include  -I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include  "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL -DVAL_CC="\"C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe\"" -DVAL_CPPFLAGS="\"-I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL\"" -DVAL_CFLAGS="\"-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g\"" -DVAL_CFLAGS_SL="\"\"" -DVAL_LDFLAGS="\"-LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -Wl,--allow-multiple-definition -Wl,--disable-auto-import -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib\"" -DVAL_LDFLAGS_EX="\"\"" -DVAL_LDFLAGS_SL="\"\"" -DVAL_LIBS="\"-lpgcommon -lpgport -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -llz4d -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lssl -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lcrypto -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lzlibd -lpthread -lws2_32 -ldl -lm -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -Wl,-Bstatic,-luuid,-Bdynamic -lcomdlg32 -ladvapi32 -lws2_32\""  -c -o config_info.o config_info.c
<builtin>: update target 'controldata_utils.o' due to: target does not exist
C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g -DFRONTEND -I. -I../../src/common -I../../src/include  -I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include  "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL -DVAL_CC="\"C:/Users/aleks/gcc/bin/x86_64-w64-mingw32-gcc.exe\"" -DVAL_CPPFLAGS="\"-I./src/include/port/win32 -IC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/../include "-I../../src/include/port/win32" -DWIN32_STACK_RLIMIT=4194304 -DBUILDING_DLL\"" -DVAL_CFLAGS="\"-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Werror=vla -Wendif-labels -Wmissing-format-attribute -Wimplicit-fallthrough=3 -Wcast-function-type -Wshadow=compatible-local -Wformat-security -fno-strict-aliasing -fwrapv -fexcess-precision=standard -Wno-format-truncation -Wno-stringop-truncation -g -g\"" -DVAL_CFLAGS_SL="\"\"" -DVAL_LDFLAGS="\"-LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -Wl,--allow-multiple-definition -Wl,--disable-auto-import -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib\"" -DVAL_LDFLAGS_EX="\"\"" -DVAL_LDFLAGS_SL="\"\"" -DVAL_LIBS="\"-lpgcommon -lpgport -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -llz4d -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lssl -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lcrypto -LC:/Users/aleks/CLionProjects/prediction-market/vcpkg_installed/x64-mingw-dynamic/debug/lib -lzlibd -lpthread -lws2_32 -ldl -lm -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -Wl,-Bstatic,-luuid,-Bdynamic -lcomdlg32 -ladvapi32 -lws2_32\""  -c -o controldata_utils.o controldata_utils.c
make[1]: Leaving directory '/c/Users/aleks/CLionProjects/prediction-market/extern/vcpkg/buildtrees/libpq/x64-mingw-dynamic-dbg/src/common'
```
</details>

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "prediction-market",
  "version-string": "0.1.0",
  "dependencies": [
    {
      "name": "drogon",
      "features": [
        "postgres"
      ]
    }
  ]
}

```
</details>
