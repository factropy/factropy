use <lib.scad>

hd = 36;
ld = 8;

module bodyHD($fn=hd) {
	translate([0,0,0.25]) rbox([0.5,0.5,0.5], 0.01, $fn=8);
}

module bodyLD($fn=ld) {
	translate([0,0,0.25]) box([0.5,0.5,0.5]);
}

bodyLD();
