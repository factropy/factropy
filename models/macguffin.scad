use <lib.scad>

module thing() {
	box([0.6,0.6,0.6]);
}

hd = 72;
ld = 8;

thing($fn=hd);
