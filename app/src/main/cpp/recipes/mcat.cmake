add_library(mcat SHARED "recipes/mcat.c")
target_link_options(mcat PRIVATE "-s")