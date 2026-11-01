#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include "third_party/yyjson/yyjson.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

#define RED     "\033[31m"
#define RESET   "\033[0m"


#define IMG_SUCCESS        0
#define ERR_IMG_UNK_ARG    1
#define ERR_IMG_NOFILE     2
#define ERR_IMG_VQ         3
#define ERR_IMG_NOPERM     4
#define ERR_IMG_PROC       5
#define ERR_IMG_NOCOL      6


#define DEBUG(dbg_msg) if(verbose) printf("[DEBUG] " dbg_msg "\n");
#define ERR_MSG(err_msg) fprintf(stderr, RED "[ERROR] " RESET err_msg "\n");



typedef struct {
    unsigned char r, g, b, a;
} color_t;

typedef struct {
    int x, y;
} pixel_t;

typedef struct {
    color_t col;
    pixel_t *pixels;
    size_t count;
} tile_t;

typedef struct {
    int x, y;
    int w, h;
} rect_t;



static void func_help(void) {
    printf("usage: img2tiles --file <file> [OPTIONS]\n");
    printf(" ");
    printf("--file, -f         point input file\n");
    printf("--help, -h         show this note\n");
    printf("--verbose, -v      debug info\n");
    printf("--quiet, -q        no output\n");
}

static void check_vq(int v, int q) {
    if(v && q) {
        ERR_MSG("can't use verbose and quiet at the same time.")
        exit(ERR_IMG_VQ);
    }
}

static void is_file_exist(char* filename) {
    if(filename == NULL) {
        ERR_MSG("file not found.")
        exit(ERR_IMG_NOFILE);
    }
}

static void has_access(char* filename) {
    if(access(filename, F_OK) == -1) {
        ERR_MSG("permission denied.")
        exit(ERR_IMG_NOPERM);
    }
}

static unsigned char* load_image(char* filename, int* w, int *h, int *ch, int img_bytes) {
    unsigned char* raw = stbi_load(filename, w, h, ch, img_bytes);
    if(!raw) {
        ERR_MSG("file processing failed.")
        exit(ERR_IMG_PROC);
    }
    return raw;
}

static color_t hex_to_rgb(char* hex_code) {
    color_t color = {0, 0, 0, 255};
    unsigned int r, g, b;

    if(hex_code[0] == '#') hex_code++;

    if(sscanf(hex_code, "%02x%02x%02x", &r, &g, &b) == 3) {
        color.r = (unsigned char)r;
        color.g = (unsigned char)g;
        color.b = (unsigned char)b;
    }
    return color;
}

static rect_t merge_pixels(pixel_t *pixels, int pixel_count, int img_width, int img_height) {
    bool* pixel_map = (bool*)calloc(img_width * img_height, sizeof(bool));

    for(size_t i = 0; i < (size_t)pixel_count; i++) {
        int idx = pixels[i].y * img_width + pixels[i].x;
        pixel_map[idx] = true;
    }

    size_t startx;
    size_t starty;
    size_t endx;
    size_t endy;

    for(size_t i = 0; i < (size_t)pixel_count; i++) {
        startx = pixels[i].x;
        starty = pixels[i].y;
        size_t start_idx = starty * img_width + startx;

        if(!pixel_map[start_idx]) {
            continue;
        }

        endx = startx;
        endy = starty;

        while((endx + 1) < img_width && pixel_map[starty * img_width + (endx + 1)]) {
            endx++;
        }

        bool can_grow_down = true;
        while(can_grow_down) {
            size_t nexty = endy + 1;
            if(nexty >= img_height) break;

            for(size_t x = startx; x <= endx; x++) {
                if(!pixel_map[starty * img_width + x]) {
                    can_grow_down = false;
                    break;
                }
            }

            if(can_grow_down) {
                endy++;
            }

            for(size_t y = starty; y <= endy; y++) {
                for(size_t x = startx; x <= endx; x++) {
                    pixel_map[y * img_width + x] = 0;
                }
            }
        }
    }

    rect_t out = {0};
    out.x = startx;
    out.y = starty;
    out.w = endx - startx + 1;
    out.h = endy - starty + 1;
    return out;
}

static char *add_extension(char *src, char *ext) {
    char *src_no_ext = strtok(src, ".");

    size_t src_no_ext_len = strlen(src_no_ext);
    size_t ext_len = strlen(ext);

    char *result = (char *)malloc(src_no_ext_len + ext_len + 1);

    strcpy(result, src_no_ext);
    strcat(result, ext);

    return result;
}

yyjson_mut_val *create_rect_json_obj(yyjson_mut_doc *rdoc, rect_t *rct, color_t *clr) {
    yyjson_mut_val *obj = yyjson_mut_obj(rdoc);

    yyjson_mut_obj_add_int(rdoc, obj, "x", rct->x);
    yyjson_mut_obj_add_int(rdoc, obj, "y", rct->y);
    yyjson_mut_obj_add_int(rdoc, obj, "w", rct->w);
    yyjson_mut_obj_add_int(rdoc, obj, "h", rct->h);

    yyjson_mut_val *color_obj = yyjson_mut_obj(rdoc);
    yyjson_mut_obj_add_int(rdoc, color_obj, "r", clr->r);
    yyjson_mut_obj_add_int(rdoc, color_obj, "g", clr->g);
    yyjson_mut_obj_add_int(rdoc, color_obj, "b", clr->b);
    yyjson_mut_obj_add_int(rdoc, color_obj, "a", clr->a);

    yyjson_mut_obj_add_val(rdoc, obj, "color", color_obj);
    return obj;
}



int main(int argc, char **argv) {
    int verbose = 0, quiet = 0;
    int width, height, channels;
    int opt;
    char *input_file = NULL;
    int status = 0;

    static struct option args[] = {
        {"file", required_argument, NULL, 'f'},
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"quiet", no_argument, NULL, 'q'}
    };

    while((opt = getopt_long(argc, argv, "f:hvq", args, NULL)) != -1) {
        switch(opt) {
            case 'f':
                input_file = optarg;
                break;
            case 'h':
                func_help();
                return 0;
            case 'v':
                verbose = 1;
                break;
            case 'q':
                quiet = 1;
                break;
            case '?':
                return ERR_IMG_UNK_ARG;
            default:
                abort();
        }
    }

    int col_count = argc - optind;
    tile_t tiles[col_count];

    if(optind < argc) {
        DEBUG("processing colors")
        while(optind < argc) {
            tiles[optind].col = hex_to_rgb(argv[optind]);
            optind++;
        }
    } else {
        ERR_MSG("no colors received.")
        return ERR_IMG_NOCOL;
    }

    DEBUG("checking args for errors...")
    check_vq(verbose, quiet);
    is_file_exist(input_file);
    has_access(input_file);

    DEBUG("opening file...")
    unsigned char* img = load_image(input_file, &width, &height, &channels, 4);

    for(size_t y = 0; y < (size_t)height; y++) {
        for(size_t x = 0; x < (size_t)width; x++) {
            size_t idx = (y * width + x) * 4;

            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];

            for(size_t i = 0; i < (size_t)col_count; i++) {
                if(r == tiles[i].col.r && g == tiles[i].col.g && b == tiles[i].col.b) {
                    size_t memc = tiles[i].count++;
                    tiles[i].pixels = realloc(tiles[i].pixels, sizeof(pixel_t) * memc);
                    tiles[i].pixels[memc].x = x;
                    tiles[i].pixels[memc].y = y;
                }
            }
        }
    }

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root_array = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, root_array);

    for(size_t i = 0; i < (size_t)col_count; i++) {
        rect_t rect = merge_pixels(tiles[i].pixels, tiles[i].count, width, height);
        yyjson_mut_val *rect_obj = create_rect_json_obj(doc, &rect, &tiles[i].col);
        yyjson_mut_arr_add_val(root_array, rect_obj);
    }


    char *output_file = add_extension(input_file, ".json");
    yyjson_write_err err;
    bool success = yyjson_mut_write_file(output_file, doc, YYJSON_WRITE_PRETTY, NULL, &err);

    if(success) {
        DEBUG("JSON file successfully generated!")
    } else {
        ERR_MSG("file write error")
        printf("%s\n", err.msg);
    }

    free(output_file);
    yyjson_mut_doc_free(doc);
    stbi_image_free(img);
    for(size_t i = 0; i < (size_t)col_count; i++) {
        free(tiles[i].pixels);
    }

    return IMG_SUCCESS;
}
