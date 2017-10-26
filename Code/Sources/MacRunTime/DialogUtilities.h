/* DialogUtilities.h */

#ifndef	DIALOG_UTILITIES_H

void			DU_show_working_window                  (short dialogID, const char *line_1_msg, const char* line_2_msg);
void			DU_remove_working_dialog                ();
SInt16          DU_CautionAlert                         (SInt16 alertID, void* upp);
SInt16          DU_StopAlert                            (SInt16 alertID, void* upp);
void            DU_ReportErr                            (const char *s);

#if !__LP64__
    void 			center_window_on_main_screen        (DialogPtr dialog);
    void 			center_window_on_parent_window      (DialogPtr dialog, WindowPtr parent);

    void			set_dialog_item_enable              (DialogPtr the_dialog, short which_item, Boolean enable);
    void			set_dialog_item_visibility          (DialogPtr the_dialog, short which_item, Boolean visibility);
    void			set_dialog_control_hilite           (DialogPtr the_dialog, short which_item, int the_hilite);

    void			set_dialog_control_value            (DialogPtr the_dialog, short which_item, int the_value);
    int				get_dialog_control_value            (DialogPtr the_dialog, short which_item);

    void			set_dialog_user_item_value          (DialogPtr the_dialog, short which_item, Handle the_value);

    void			set_dialog_text_value               (DialogPtr the_dialog, short which_item, char *the_str);
    void			set_dialog_text_special             (DialogPtr the_dialog, short which_item, char *the_str);
    void			get_dialog_text_value               (DialogPtr the_dialog, short which_item, char *the_str);

    pascal void 	draw_box_around_item_rect           (DialogPtr dialog, short itemNo);
    pascal void 	draw_frame_around_button            (DialogPtr dialog, short itemNo);
    pascal void 	draw_frame_around_working_message   (DialogPtr dialog, short itemNo);

    void			shift_dialog_contents               (DialogPtr dialog, short delta_h, short delta_v, int num_items_plus_one);
#endif

#endif