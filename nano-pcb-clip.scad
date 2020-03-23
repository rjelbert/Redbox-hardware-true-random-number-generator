
difference(){
// add stuff
group(){
translate([0,0,0])
    cube([6,(18+6),3]);

translate([-8,(18+6),-9])
cube([14,2,12]);
    
}

// take away
group(){
translate([3,2,0])
    cube([3,18,1.5]);
}
}

hull(){
translate([0,0,0])
    cube([1,1,3]);

translate([-8,(18+6),0])
    cube([8,2,3]);
}