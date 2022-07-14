use <lib.scad>;

hd = 72;
ld = 16;
vld = 8;

module tower() {
	union() {
		translate([0,0,7.5]) cyl(0.5, 0.4, 15.0);
		translate([0,0,1]) cyl(1, 1, 2);
	}
}

module blade() {
	translate([0,-5,8.5]) rotate([0,90,0])
		scale([0.9,0.1,3]) parabola(1,0,0, .3, -7, 7, 10);
}

//tower($fn=vld);
blade($fn=vld);