use <lib.scad>

hd = 16;

module beaconFrameHD($fn=hd) {
	translate([0,0,0.05]) difference() {
		cyl(0.13,0.11,0.1);
		cyl(0.12,0.10,0.085);
		translate([0,0,0.1]) cyl(0.095,0.095,0.2);
		for (i = [0:1:1]) rotate([0,0,i*90]) hull() {
			translate([0,0,0.037]) box([0.14,0.3,0.001]);
			translate([0,0,-0.035]) box([0.16,0.3,0.001]);
		}
	}
}

module beaconGlassHD($fn=hd) {
	translate([0,0,0.05]) cyl(0.115,0.0975,0.08);
}

//beaconFrameHD();
beaconGlassHD();
