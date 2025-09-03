#pragma once
#include <string>
#include <vector>

// TODO: investigate replacing these with just a std::pair<std::string, std;:string>
// and returning a std::span<std::pair<std::string_view, std::string_view>>

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
    char *keyword;                 /* Keyword in look/examine          */
    char *description;             /* What to see                      */
    struct extra_descr_data *next; /* Next in list                     */
};

// new variant of extra_descr_data that uses std::string
struct ExtraDescription {
    std::string keyword;          /* Keyword in look/examine          */
    std::string description;      /* What to see                      */
};


struct HasExtraDescriptions {
    const std::vector<ExtraDescription>& getExtraDescription() const; // Returns the extra description data.    
    std::vector<ExtraDescription> extra_descriptions; // Extra descriptions for this unit.
};

extern void free_extra_descriptions(struct extra_descr_data *edesc);