SET(SOURCES ui_comp_menubar.c
    ui_comp_statusbtn.c
    ui_comp.c
    ui_Screen1.c
    ui_Screen2.c
    ui_Screen3.c
    ui_Screen4.c
    ui_Screen5.c
    ui_Screen6.c
    ui_Screen7.c
    ui_Screen8.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_new_background_png.c
    ui_img_status_png.c
    ui_img_selected_bg_png.c
    ui_img_food_png.c
    ui_img_game_png.c
    ui_img_light_png.c
    ui_img_med_png.c
    ui_img_poop_png.c
    ui_img_spicy_png.c
    ui_img_alert_png.c
    ui_img_an_chon1_png.c
    ui_img_pat1_png.c
    ui_img_pat2_png.c
    ui_img_pat3_png.c
    ui_img_pat5_png.c
    ui_img_an_chon2_png.c)

add_library(ui ${SOURCES})
