use <lib.scad>

//#box([21,21,1]);
//#box([9,9,1]);

module straight() {
	translate([0,0,0.05]) box([3,1,0.1]);
}

module diagonal() {
	translate([0,0,0.05]) rotate([0,0,45]) box([5,1,0.1]);
}

module cornerLeftHD($fn=72) {
	radius=26.5;
	translate([-radius-1.5,-2.5,0]) rotate([0,0,0.35]) rotate_extrude(angle=45) translate([radius,0,0]) square([3,0.1]);
}

module cornerRightHD() {
	mirror([1,0,0]) cornerLeftHD();
}

module cornerRightLD() {
	mirror([1,0,0]) cornerLeftLD();
}

//for (i = [0:1:3]) translate([0,i*9,0]) straight();
//translate([0,27,0]) rotate([0,0,-90]) for (i = [0:1:3]) translate([0,i*9,0]) straight();
//straight();
//diagonal();
cornerLeftHD();
//cornerRightHD();
//translate([0,0,0.5]) cornerRightHD();
//translate([0,0,0.5]) cornerRightHD();
//translate([0,-9,0.5]) cornerRightHD();
//translate([-20,20,0]) diagonal();
//translate([-40,40,0]) rotate([0,0,45]) cornerLeftHD();
//translate([-40,40,0]) rotate([0,0,180]) cornerLeftHD();
//cornerRightHD();
