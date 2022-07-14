use <lib.scad>

hd = 72;
ld = 16;
vld = 8;

module platform() {
	translate([0,0,-4.75]) union() {
		translate([0,0,4.5]) box([9,9,0.5]);
		translate([3,3,0]) cyl(1,1,9);
		translate([3,-3,0]) cyl(1,1,9);
		translate([-3,3,0]) cyl(1,1,9);
		translate([-3,-3,0]) cyl(1,1,9);
		translate([0,3,4]) box([6,0.5,1]);
		rotate([0,0,90]) translate([0,3,4]) box([6,0.5,1]);
		translate([0,-3,4]) box([6,0.5,1]);
		rotate([0,0,90]) translate([0,-3,4]) box([6,0.5,1]);
	}
}

module tower() {
	translate([0,0,4]) union() {
		translate([0,0,4]) box([0.75,0.75,0.75]);
		translate([0,0,-4]) box([4,4,0.75]);

		for (i = [0:1:3])
			rotate([0,0,i*90+45]) translate([0,1.4,0]) rotate([17,0,0]) box([0.2,0.2,8.25]);

		if ($fn > vld) {
			translate([0,0,-3]) for (i = [0:1:3])
				rotate([0,0,i*90]) translate([1.62,0,0]) box([0.1,3.4,0.1]);
			translate([0,0,-1]) for (i = [0:1:3])
				rotate([0,0,i*90]) translate([1.22,0,0]) box([0.1,2.5,0.1]);
			translate([0,0,1]) for (i = [0:1:3])
				rotate([0,0,i*90]) translate([0.81,0,0]) box([0.1,1.6,0.1]);

			for (i = [0:1:3])
				rotate([0,0,i*90]) translate([0.95,0,0.2]) rotate([0,-12,0]) rotate([47,0,0]) translate([0,-0.3,0]) box([0.1,2.9,0.1]);

			for (i = [0:1:3])
				rotate([0,0,i*90]) translate([1.4,0,-1.9]) rotate([0,-12,0]) rotate([-36,0,0]) translate([0,0.3,0]) box([0.1,3.4,0.1]);
		}
	}
}

//platform($fn=vld);
tower($fn=vld);