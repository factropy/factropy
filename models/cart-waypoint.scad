use <lib.scad>

hd = 72;
ld = 12;

module padHD() {
	rotate([0,0,22.5]) cyl(0.2, 0.2, 0.03, $fn=8);
}

module edgeHD() {
	rotate([0,0,22.5]) difference() {
		cyl(0.25, 0.25, 0.03, $fn=8);
		cyl(0.20, 0.20, 0.1, $fn=8);
	}
}

padHD($fn=hd);
//edgeHD($fn=hd);
