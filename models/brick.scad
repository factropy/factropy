use <lib.scad>

module body() {
	translate([0,0,0.15]) difference() {
		if ($fn > 8) rbox([0.65, 0.45, 0.3], 0.01, $fn=round($fn/8)); else box([0.65, 0.45, 0.3]);
		translate([0.19,0.11,0]) cyl(0.055, 0.055, 1.0);
		translate([0.0,0.11,0]) cyl(0.055, 0.055, 1.0);
		translate([-0.19,0.11,0]) cyl(0.055, 0.055, 1.0);
		translate([0.19,-0.11,0]) cyl(0.055, 0.055, 1.0);
		translate([0.0,-0.11,0]) cyl(0.055, 0.055, 1.0);
		translate([-0.19,-0.11,0]) cyl(0.055, 0.055, 1.0);
	}
}

d = 4;
body($fn=d);
