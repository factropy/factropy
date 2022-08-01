use <lib.scad>

hd = 24;

module smrFinsHD($fn=hd) {
	difference() {
		for (i = [0:45:360]) rotate([0,0,i+22.5]) hull() {
			translate([5,0,2.5]) rotate([90,0,0]) cyl(1,1,0.1);
			translate([5,0,7.5]) rotate([90,0,0]) cyl(1,1,0.1);
			translate([0,0,5]) box([1,0.1,7]);
		}
		translate([0,0,3]) difference() { cyl(10,10,0.1); cyl(4,4,0.2); }
		translate([0,0,5]) difference() { cyl(10,10,0.1); cyl(4,4,0.2); }
		translate([0,0,7]) difference() { cyl(10,10,0.1); cyl(4,4,0.2); }
		translate([0,0,5]) cyl(3,3,8);
		translate([0,0,8.5]) hull() {
			translate([0,0,2.5]) cyl(5,5,1);
			translate([0,0,-2.5]) cyl(1,1,1);
		}
	}
}

module smrBodyHD($fn=hd) {
	union() {
		translate([0,0,4.15]) difference() {
			cyl(3,3,7);
			cyl(2.9,2.9,6.8);
			for (i = [0:45:180]) hull() {
				rotate([0,0,i]) translate([3,0,0]) hull() {
					translate([0,0.5,-3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
					translate([0,-0.5,-3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
					translate([0,0.5,3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
					translate([0,-0.5,3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
				}
				rotate([0,0,i+180]) translate([3,0,0]) hull() {
					translate([0,0.5,-3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
					translate([0,-0.5,-3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
					translate([0,0.5,3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
					translate([0,-0.5,3]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
				}
			}
		}
		translate([0,0,4.15]) cyl(2,2,6.8);
	}
}

module smrBaseHD($fn=hd) {
	translate([0,0,0.35]) cyl(4,4,0.7);
}

module smrCapHD($fn=hd) {
	translate([0,0,7.8]) intersection() {
		union() {
			translate([0,0,-1]) for (i = [0:45:360])
				rotate([0,0,i+22.5]) rotate([90,0,0]) cyl(3,3,0.2);
			hull() {
				cyl(3,3,0.5);
				translate([0,0,0.5]) cyl(2.6,2.6,0.5);
			}
			hull() {
				translate([0,0,3]) cyl(1,1,0.1);
				cyl(2,2,4);
			}
		}
		translate([0,0,1.1]) box([10,10,2.2]);
	}
}

module smrSpinnerHD($fn=hd) {
	translate([0,0,4.15]) union() {
		box([5.5,1,6]);
		rotate([0,0,45]) box([5.5,1,6]);
		rotate([0,0,90]) box([5.5,1,6]);
		rotate([0,0,135]) box([5.5,1,6]);
	}
}

module smrPipesHD($fn=hd/2) {
	x = 3.5;
	z = 8;
	points = [
		[x,0,-1],
		[x,0,(z-0.2)],
		[(x-0.07),0,(z-0.07)],
		[(x-0.2),0,z],
		[-(x-0.2),0,z],
		[-(x-0.07),0,(z-0.07)],
		[-x,0,(z-0.2)],
		[-x,0,-1],
	];
	union() {
		rotate([0,0,0]) translate([0,0,1.5]) pipe(points, 0.3);
		rotate([0,0,90]) translate([0,0,1.5]) pipe(points, 0.3);
	}
}

//smrCapHD();
//smrBodyHD();
//smrBaseHD();
//smrFinsHD();
smrSpinnerHD();
//smrPipesHD();