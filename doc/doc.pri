
# DOXYGEN_BIN=$$findFile(doxygen)
DOXYGEN_BIN=doxygen

QMAKE_EXTRA_UNIX_TARGETS += doc
doc.target = doc
isEmpty(DOXYGEN_BIN) {
    doc.commands = @echo "Unable to detect doxygen in PATH"
} else {
    doc.commands = @$${DOXYGEN_BIN} doc/duidoxy.cfg ;
    #doc.commands += ./doc/xmlize.pl ;
}
doc.depends = FORCE
