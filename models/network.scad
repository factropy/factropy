use <lib.scad>

module repeater() {
	union() {
		translate([0,0,3]) cyl(0.1, 0.1, 6);

		for (i = [0:1:2])
			rotate([0,0,i*120]) translate([0,1,5.5]) union() {
				box([0.5,0.1,1.0]);
				translate([0,-0.5,0]) box([0.1,1,0.1]);
			}

		rotate([0,0,-30]) translate([0,0,5.5]) difference() {
			cyl(1,1,0.1,$fn=3);
			cyl(0.9,0.8,0.2,$fn=3);
		}

		translate([0,0,0.25]) cyl(0.4,0.4,0.5);
	}
}

repeater($fn=72);