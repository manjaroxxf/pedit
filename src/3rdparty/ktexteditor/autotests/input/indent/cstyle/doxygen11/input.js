v.setCursorPosition(0,0);
v.selectAll();

var r = v.selection();
if (r.isValid()) {
    v.align(r);
}
