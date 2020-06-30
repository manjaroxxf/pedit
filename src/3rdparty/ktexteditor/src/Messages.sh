#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp || exit 11
#$EXTRACTATTR --attr=language,name,Language --attr="language,section,Language Section" syntax/data/*.xml >> rc.cpp || exit 12
grep -n -e '^ *"name":' script/data/indentation/*.js | sed 's!^\(.*\):.*"name": *"\(.*\)".*$!// i18n: file: \1\ni18nc("Autoindent mode", "\2");!' | sed 's/ \+")/")/' >>rc.cpp || exit 13
grep -n -e '^ *"name":' script/data/commands/*.js | sed 's!^\(.*\):.*"name": *"\(.*\)".*$!// i18n: file: \1\ni18nc("Script command name", "\2");!' | sed 's/ \+")/")/' >>rc.cpp || exit 13
grep -n -e '^ *"category":' script/data/commands/*.js | sed 's!^\(.*\):.*"category": *"\(.*\)".*$!// i18n: file: \1\ni18nc("Script command category", "\2");!' | sed 's/ \+")/")/' >>rc.cpp || exit 13
$XGETTEXT `find . -name "*.cpp" -o -name "*.h" -o -name '*.js'` -o $podir/ktexteditor5.pot
