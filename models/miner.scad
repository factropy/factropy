use <lib.scad>

bounds = [4.99,14.99,5];

module chassisLD() {
	difference() {
		box(bounds);
		translate([0,-y(bounds)/1.2,z(bounds)*1.15]) rotate([-30,0,0]) box([x(bounds)*1.1, y(bounds), z(bounds)*1.5]);
		translate([0,-y(bounds)/3.5,0]) difference() {
			rotate([90,0,0]) cyl(x(bounds)/2.5, x(bounds)/2.5, y(bounds)/2.2, $fn=24);
			translate([0,-y(bounds)/1.5,-z(bounds)/2]) box(bounds);
		}
		translate([0,-y(bounds)*0.471,-1]) scale([1.5,1,1]) rotate([-60,0,0]) rotate([0,0,45]) cyl(0.55, 0.15, 1.5, $fn=4);
	}
}

module chassisHD() {
	minkowskiOutsideRound(0.01,$fn=8) chassisLD();
}

//chassisHD();
hull() chassisLD();