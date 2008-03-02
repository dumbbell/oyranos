#!/bin/bash -xv

# exportsymbols.sh
# ----------------
#
# Export kdbBackendFactory entry point symbols
# for each compiled backend
#
# (c) 2006 Yannick Lecaillez

func=kdbBackendFactory
toolsfuncs='ksFromXMLfile ksFromXML'
export_file=exported_symbols.c


echo '
/* exported_symbols.c generated by exportsymbols.sh */

#include "kdbbackend.h"
#include "kdbLibLoader.h"

/* Make the compiler happy */
' > $export_file

for lib in "$@"
do
	if test $lib = libelektratools; then
		for toolsfunc in $toolsfuncs; do
			symbol=${lib}_LTX_${toolsfunc}
			echo "extern int $symbol;" >> $export_file
		done
	else
		symbol=libelektra_${lib}_LTX_${func}
		echo "Exporting symbols for $lib ..."
		echo "extern int $symbol;" >> $export_file
	fi
done

echo '
kdblib_symbol kdb_exported_syms[] =
{
' >> $export_file

for lib in "$@"
do
	if test $lib = libelektratools; then
		fname=libelektratools
		echo -e "\t{\"$fname\", NULL}," >> $export_file
		for toolsfunc in $toolsfuncs; do
			symbol=${fname}_LTX_${toolsfunc}
			echo -e "\t{\"$toolsfunc\", &$symbol }," >> $export_file
		done
	else
		fname="libelektra-${lib}"
		symbol=libelektra_${lib}_LTX_${func}
		echo -e "\t{\"$fname\", NULL}," >> $export_file
		echo -e "\t{\"$func\", &$symbol }," >> $export_file
	fi
	echo "" >> $export_file
done
echo -e "\t{ NULL, NULL }" >> $export_file
echo "};" >> $export_file
