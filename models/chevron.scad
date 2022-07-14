use <lib.scad>

module chevronLD() {
	translate([0,0.05,0]) rotate([0,0,-90]) cyl(0.3,0.3,0.05,$fn=3);
}

module chevronHD() {
	minkowskiOutsideRound(0.01,$fn=8)
	chevronLD();
}

chevronHD();
