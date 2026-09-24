// Glucose Monitor Prototype -- 3D-printable enclosure
// Two parts: BASE (component box) + LID (OLED window, screws down onto BASE)
// FDM-friendly: no supports needed if printed as shown (base opening up, lid flat).
//
// ASSUMPTIONS -- verify against your actual parts before printing and adjust
// the variables below. Dimensions here are sized generously around:
//   - ESP32-C3 "SuperMini" style board (~23 x 18mm) mounted on two ribs
//   - a small universal perfboard/protoboard (~60 x 40mm) for the op-amp
//     channels, resistors, and bias divider
//   - a generic 0.96" SSD1306 I2C OLED module (~27-29mm square PCB,
//     ~24 x 13mm visible display area) -- these vary by supplier, so the
//     OLED pocket is a flush recess (module taped/glued in) rather than
//     relying on specific mounting-hole positions.
//   - USB-C port cutout on one short wall for charging/programming
//   - two 5mm wire feedthrough holes on the opposite wall for the
//     sweat/urine electrode lead pairs
//
// Render / export (also see build.sh):
//   openscad -o base.stl  -D 'part="base"' enclosure.scad
//   openscad -o lid.stl   -D 'part="lid"'  enclosure.scad
//   openscad -o preview.png -D 'part="assembly"' --camera=100,-100,90,0,0,0,0 --viewall enclosure.scad

part = "assembly"; // "base" | "lid" | "assembly" (preview only, do not print assembly)

// ---------------------------------------------------------------
// Core dimensions (mm) -- edit these to match your actual parts
wall        = 2.2;     // wall thickness
in_l        = 70;      // internal cavity length (X)
in_w        = 48;      // internal cavity width (Y)
in_h        = 18;      // internal cavity depth (Z), base only
lid_h       = 2.4;     // lid panel thickness
corner_r    = 3;       // outer corner rounding radius

// Screw posts (M3 self-tapping into PLA/PETG)
post_od       = 6.2;
post_pilot_d  = 2.6;   // pilot hole for M3 self-tap screw
post_inset    = 6;     // distance of post center from each internal corner
lid_hole_d    = 3.4;   // clearance hole in the lid for the M3 screw

// OLED pocket (flush recess in the lid, module glued/taped in from inside)
oled_pcb_w      = 29.5;
oled_pcb_h      = 29.5;
oled_pocket_d   = 1.4;    // pocket depth = PCB thickness, so the module sits flush
oled_window_w   = 25.5;   // cut all the way through, sized for the visible glass
oled_window_h   = 15.5;
oled_offset_x   = -14;    // OLED position relative to lid center (X)
oled_offset_y   = 6;      // OLED position relative to lid center (Y)

// USB-C cutout (short wall, +X side), sized with tolerance around the connector
usb_w = 10.5;
usb_h = 5.0;
usb_z = wall + 6;         // height of cutout center above the floor

// Electrode wire feedthroughs (short wall, -X side) -- one pair of holes
// per channel (sweat, urine), sized for a twisted-pair lead bundle
wire_hole_d = 5.0;
wire_hole_spacing = 14;

// ESP32-C3 mounting ribs (simple ledge, board rests on top and is held by
// friction / a dab of hot glue -- adjust rib positions to your exact board)
esp_rib_w = 4;
esp_rib_h = 6;           // rib height above floor
esp_rib_len = 20;
esp_pos_x = in_l/2 - 16;  // ribs placed toward the USB-C end
esp_pos_y = 0;

$fn = 64;

// ---------------------------------------------------------------
module rounded_box(l, w, h, r) {
    hull() {
        for (sx = [-1, 1]) for (sy = [-1, 1])
            translate([sx*(l/2 - r), sy*(w/2 - r), 0])
                cylinder(h=h, r=r);
    }
}

module screw_post(h) {
    difference() {
        cylinder(h=h, d=post_od);
        translate([0,0,-1]) cylinder(h=h+2, d=post_pilot_d);
    }
}

post_positions = [
    [ in_l/2 - post_inset,  in_w/2 - post_inset],
    [-in_l/2 + post_inset,  in_w/2 - post_inset],
    [ in_l/2 - post_inset, -in_w/2 + post_inset],
    [-in_l/2 + post_inset, -in_w/2 + post_inset],
];

// ---------------------------------------------------------------
module base() {
    out_l = in_l + wall*2;
    out_w = in_w + wall*2;
    out_h = in_h + wall;

    difference() {
        // outer shell (rounded_box already spans z=[0, out_h])
        rounded_box(out_l, out_w, out_h, corner_r);

        // hollow interior (open top)
        translate([0,0,wall + in_h/2 + 0.01])
            cube([in_l, in_w, in_h + 1], center=true);

        // USB-C cutout, +X wall
        translate([in_l/2 + wall/2, 0, usb_z])
            cube([wall + 2, usb_w, usb_h], center=true);

        // electrode wire feedthroughs, -X wall
        for (s = [-1, 1])
            translate([-in_l/2 - wall/2, s*wire_hole_spacing/2, wall + wire_hole_d/2 + 2])
                rotate([0,90,0])
                    cylinder(h=wall+2, d=wire_hole_d);
    }

    // screw posts (rise from floor)
    post_h = in_h - 1;
    for (p = post_positions)
        translate([p[0], p[1], wall])
            screw_post(post_h);

    // ESP32-C3 support ribs
    for (s = [-1, 1])
        translate([esp_pos_x, esp_pos_y + s*8, wall + esp_rib_h/2])
            cube([esp_rib_len, esp_rib_w, esp_rib_h], center=true);
}

// ---------------------------------------------------------------
module lid() {
    out_l = in_l + wall*2;
    out_w = in_w + wall*2;

    difference() {
        rounded_box(out_l, out_w, lid_h, corner_r);

        // OLED through-window (spans the full lid thickness, with margin
        // on both ends so the cut is clean at the top and bottom faces)
        translate([oled_offset_x, oled_offset_y, lid_h/2])
            cube([oled_window_w, oled_window_h, lid_h + 0.4], center=true);

        // OLED pocket, recessed from the INSIDE face (z=0, the face that
        // faces into the box) so the module glues in from inside with its
        // screen facing out through the window; the remaining lid_h-pocket_d
        // of material forms the visible outer bezel around the window.
        translate([oled_offset_x, oled_offset_y, oled_pocket_d/2])
            cube([oled_pcb_w, oled_pcb_h, oled_pocket_d + 0.2], center=true);

        // screw clearance holes
        for (p = post_positions)
            translate([p[0], p[1], -0.01])
                cylinder(h=lid_h + 0.2, d=lid_hole_d);
    }
}

// ---------------------------------------------------------------
if (part == "base") {
    base();
} else if (part == "lid") {
    lid();
} else {
    // assembly preview only -- export base.stl / lid.stl separately for printing
    color("SteelBlue") base();
    translate([0, 0, in_h + wall + 12])
        color("Plum") lid();
}
