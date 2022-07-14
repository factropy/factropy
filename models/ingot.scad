use <lib.scad>

s=0.18;

module ingotHD() {
	minkowskiOutsideRound(0.01,$fn=6) scale([s,s,s]) scale([3,1,1]) rotate([0,0,45]) cyl(1, 0.8, 1, $fn=4);
}

module ingotLD() {
	scale([s,s,s]) scale([3,1,1]) rotate([0,0,45]) cyl(1, 0.8, 1, $fn=4);
}

translate([0,-0.175,0.09]) ingotLD();
translate([0,0,0.271]) ingotLD();
translate([0,0.175,0.09]) ingotLD();
