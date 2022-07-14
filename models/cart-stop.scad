use <lib.scad>

module padHD() {
	difference() {
		rotate([0,0,22.5]) cyl(0.85, 0.85, 0.05, $fn=8);
		for (y = [-0.6:0.2:0.7]) translate([0,y,0.03]) rotate([0,90,0]) cyl(0.02,0.015,2,$fn=4);
		for (x = [-0.6:0.2:0.7]) translate([x,0,0.03]) rotate([90,0,0]) cyl(0.02,0.015,2,$fn=4);
	}
}

module edgeHD() {
	rotate([0,0,22.5]) difference() {
		cyl(0.95, 0.95, 0.03, $fn=8);
		cyl(0.85, 0.85, 0.04, $fn=8);
	}
}

module padLD() {
	difference() {
		rotate([0,0,22.5]) cyl(0.85, 0.85, 0.05, $fn=8);
	}
}

module edgeLD() {
	edgeHD();
}

padHD();
//padLD();
//edgeHD();
//edgeLD();
