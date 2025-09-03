#pragma once

struct descriptor_data;

extern void tedit_string_cleanup(struct descriptor_data *d, int terminator);

extern void news_string_cleanup(struct descriptor_data *d, int terminator);
