#!/bin/sh
#
# update-catkeys.sh
#
# Run this any time a B_TRANSLATE_CONTEXT() string is added, changed, or
# removed anywhere in Hare's own sources, libHare/LibHareStrings.h, or
# src/Encoders/EncoderStrings.h.
#
# What it does:
#   1. Regenerates locales/en.catkeys from the current sources (via
#      "make -f Makefile_Hare catkeys" - see that target's own comment
#      for why libHare/the encoders are folded into Hare's catalog).
#   2. For every *other* locales/<lang>.catkeys file already checked in
#      (a real translation, not the "en" template), merges in whatever
#      changed:
#        - New strings: appended as new lines with an empty translation,
#          ready for a translator to fill in.
#        - Removed strings: dropped from the file entirely (a stale
#          translation for a string that no longer exists can't be
#          matched back up to anything anyway).
#        - Everything else: left exactly as it was, translation and all.
#      It also copies over en.catkeys's own header line, since that
#      line's fingerprint is what tells Haiku's catalog loader whether a
#      given locales/<lang>.catkeys still matches this build's set of
#      strings - leaving a translation file's old header in place after
#      the string set changes is exactly what makes Haiku silently
#      ignore that translation at runtime instead of using it.
#
# A key is the (original text, context, comment) triplet - i.e. every
# column of a catkeys line except the translation itself - so a line
# only counts as "the same string" across two catkeys files if all
# three match.
#
# Nothing here touches locales/en.catkeys's own translations (it doesn't
# have any - it's the template - see the "catkeys" target's comment in
# Makefile_Hare) or asks a translator for anything; it just keeps every
# checked-in translation in sync with whatever the source now says.
#
# Safe to run with no other locales/*.catkeys files present yet (nothing
# to merge, so it just regenerates locales/en.catkeys and says so).

set -e

cd "$(dirname "$0")"

LOCALES_DIR="locales"
EN_CATKEYS="$LOCALES_DIR/en.catkeys"

echo "Regenerating $EN_CATKEYS from current sources..."
mkdir -p "$LOCALES_DIR"

make -f Makefile_Hare OBJ_DIR="objects_hare" catkeys

if [ ! -f "$EN_CATKEYS" ]; then
	echo "ERROR: $EN_CATKEYS still doesn't exist after running the" >&2
	echo "catkeys target - check that collectcatkeys is on your PATH." >&2
	exit 1
fi

# merge_one_locale <new-en.catkeys> <lang.catkeys>
#
# Rewrites <lang.catkeys> in place:
#   - line 1 becomes <new-en.catkeys>'s own header line
#   - every remaining line of <new-en.catkeys> gives one output line:
#       - if <lang.catkeys> already had a line with that same key,
#         that line (translation included) is reused as-is
#       - otherwise a new line is written with an empty translation
#   - any line that was in <lang.catkeys> but has no matching key in
#     <new-en.catkeys> is left out (a stale translation for a string
#     that's gone)
merge_one_locale() {
	new_en="$1"
	lang_file="$2"

	awk -F'\t' -v OFS='\t' '
		# First file (the new en.catkeys): remember its header, and
		# the order its keys appear in.
		NR == FNR {
			if (FNR == 1) {
				new_header = $0
			} else {
				key = $1 "\t" $2 "\t" $3
				if (!(key in seen_key)) {
					seen_key[key] = 1
					key_order[++key_count] = key
				}
			}
			next
		}
		# Second file (the language file being merged): skip its old
		# header (we are replacing it), and remember each existing
		# line by key so it can be reused below instead of being
		# rewritten as a blank translation.
		FNR == 1 { next }
		{
			key = $1 "\t" $2 "\t" $3
			if (key in seen_key) {
				lang_line[key] = $0
				kept++
			} else {
				dropped++
			}
		}
		END {
			print new_header
			for (i = 1; i <= key_count; i++) {
				key = key_order[i]
				if (key in lang_line) {
					print lang_line[key]
				} else {
					print key "\t"
					added++
				}
			}
			printf("  %s: kept %d, added %d, dropped %d\n",
				ARGV[2], kept + 0, added + 0, dropped + 0) > "/dev/stderr"
		}
	' "$new_en" "$lang_file" > "$lang_file.new"

	mv "$lang_file" "$lang_file.bak"
	mv "$lang_file.new" "$lang_file"
}

found_other_locale=0
for lang_file in "$LOCALES_DIR"/*.catkeys; do
	[ -f "$lang_file" ] || continue
	base=$(basename "$lang_file")
	[ "$base" = "en.catkeys" ] && continue
	found_other_locale=1
	merge_one_locale "$EN_CATKEYS" "$lang_file"
done

if [ "$found_other_locale" = "0" ]; then
	echo "No other locales/*.catkeys files found - nothing to merge yet."
	echo "(locales/en.catkeys is just the template; see the \"catkeys\""
	echo "target's comment in Makefile_Hare.)"
else
	echo "Updated translation file(s) above - each old copy was kept"
	echo "alongside it as <file>.bak in case you want to double check"
	echo "the merge before removing it."
fi
