use <lib.scad>;

radius = 0.35;
height = 0.8;

module drumHD($fn=24) {
	module ring() {
		torus(0.01,radius-0.0075);
	}
	module cutOut() {
		translate([radius*0.5,0,height*0.5+0.03]) hull() {
			cyl(0.1,0.1,0.05);
			cyl(0.08,0.08,0.1);
		}
	}
	translate([0,0,height/2]) difference() {
		union() {
			cyl(radius,radius,height);
			translate([0,0,height/2]) ring();
			translate([0,0,height/5.75]) ring();
			translate([0,0,-height/5.75]) ring();
			translate([0,0,-height/2]) ring();
		}
		cutOut();
	}
}

module lidHD($fn=24) {
	translate([radius*0.5,0,height-0.01]) cyl(0.07,0.07,0.05);
}

module drumLD($fn=12) {
	module ring() {
		torus(0.01,radius-0.0075);
	}
	module cutOut() {
		translate([radius*0.5,0,height*0.5+0.03]) hull() {
			cyl(0.1,0.1,0.05);
			cyl(0.08,0.08,0.1);
		}
	}
	translate([0,0,height/2]) difference() {
		union() {
			cyl(radius,radius,height);
			translate([0,0,height/2]) ring();
			translate([0,0,height/5.75]) ring();
			translate([0,0,-height/5.75]) ring();
			translate([0,0,-height/2]) ring();
		}
		cutOut();
	}
}

module lidLD($fn=12) {
	translate([radius*0.5,0,height-0.01]) cyl(0.07,0.07,0.05);
}

//drumHD();
//lidHD();
drumLD();
//lidLD();
