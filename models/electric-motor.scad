use <lib.scad>

module body() {
	translate([0,0,0.25]) rotate([90,0,0]) cyl(0.2, 0.2, 0.4);
}

module foot() {
	translate([0,0,0.1]) rotate([0,-90,0]) cyl(0.2, 0.2, 0.3, $fn=3);
}

module shaft() {
	translate([0,0,0.25]) rotate([90,0,0]) cyl(0.04, 0.04, 0.5);
}

hd = 16;
ld = 8;

//body($fn=hd);
//body($fn=ld);
//foot($fn=hd);
//foot($fn=ld);
shaft($fn=hd);
//shaft($fn=ld);
