#pragma once
#include "memory.h"
#include <string.h>


static const double _pow10[23] = {
    1e-11,1e-10,1e-9,1e-8,1e-7,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1,
    1e0,
    1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11
};

#define POW10(p) _pow10[(p) + 11]

// ──────────────────────────────────────────────
//  Структуры данных (без изменений)
// ──────────────────────────────────────────────
typedef struct { double x, y, z;    } Vec3;
typedef struct { double x, y, z, w; } Vec4;
typedef struct { double x, y, z;    } Uv3;
typedef struct { double x, y, z;    } Normal;
typedef struct { uint32_t v, uv, n; } Vertex;

typedef struct {
    char  *way;
    Vec3   s, o, t;
    bool   clamp, blendu, blendv, cc;
    double bm;
    char   imf;
    char  *type;
} Mtl_map;

typedef struct {
    char    *name;
    double   Ns, Ni, opacity;
    Vec3     Ka, Kd, Ks, Ke;
    Mtl_map *map_Ns, *map_Ka, *map_Kd, *map_Ks;
    Mtl_map *map_opacity, *map_bump;
    int8_t   illum;
} Mtl;

typedef struct {
    int      size_v;   Vec4   *vectors;
    int      size_uv;  Uv3    *uvs;
    int      size_n;   Normal *normals;
    int      size_mtl; Mtl    *material;
    Vertex **tringles;
    int      size_com; char   *coments;
    char    *mtllib;
    char    *obj_name;
    int8_t   s;
} Parse_OBJ;


static inline int skip_spaces(const char *s) {
    int i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    return i;
}

static inline int parse_int(const char *s, int *out) {
    int i = 0;
    int sign = 1;
    if (s[i] == '-') { sign = -1; i++; }
    int val = 0;
    while (s[i] >= '0' && s[i] <= '9')
        val = val * 10 + (s[i++] - '0');
    *out = val * sign;
    return i;
}

static inline int parse_double(const char *s, double *out) {
    int i = 0;
    double sign = 1.0;
    if (s[i] == '-') { sign = -1.0; i++; }

    double val = 0.0;
    while (s[i] >= '0' && s[i] <= '9')
        val = val * 10.0 + (s[i++] - '0');

    if (s[i] == '.') {
        i++;
        double frac = 0.1;
        while (s[i] >= '0' && s[i] <= '9') {
            val += (s[i++] - '0') * frac;
            frac *= 0.1;
        }
    }

    if (s[i] == 'e' || s[i] == 'E') {
        i++;
        int exp = 0;
        int exp_sign = 1;
        if (s[i] == '-') { exp_sign = -1; i++; }
        else if (s[i] == '+') i++;
        while (s[i] >= '0' && s[i] <= '9')
            exp = exp * 10 + (s[i++] - '0');
        exp *= exp_sign;
        if (exp >= -11 && exp <= 11) val *= POW10(exp);
        else {
            double p = 1.0;
            if (exp > 0) { while (exp--) p *= 10.0; val *= p; }
            else         { while (exp++) p *= 10.0; val /= p; }
        }
    }

    *out = val * sign;
    return i;
}

static inline int parse_doubles(const char *s, double *out, int *n) {
    int i = 0; *n = 0;
    while (*n < 4) {
        i += skip_spaces(s + i);
        if (s[i] == '\n' || s[i] == '\r' || s[i] == '\0') break;
        if (s[i] != '-' && (s[i] < '0' || s[i] > '9')) break;
        int consumed = 0;
        consumed = parse_double(s + i, &out[(*n)++]);
        if (consumed == 0) break;
        i += consumed;
    }
    return i;
}


static inline int copy_until(const char *src, char *dst, char term) {
    int i = 0;
    while (src[i] && src[i] != term && src[i] != '\r') {
        dst[i] = src[i]; i++;
    }
    dst[i] = '\0';
    return i;
}


#define MATCH(data, lit) (meta_if((void*)(data), (void*)(lit), sizeof(lit)-1))


static inline int skip_line(const char *s) {
    int i = 0;
    while (s[i] && s[i] != '\n') i++;
    return i;
}


static char *file_read(const char *filename) {
    FILE* file = fopen(filename, "rb+");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    uint64_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length == 0) {
		printf("file: %s;\n", filename);
        meta_error("not curect length");
        fclose(file);
        return NULL;
    }

    char* buffer = meta_malloc((size_t)length + 1);
    if (buffer != NULL) {
		fread(buffer, 1, length, file);
        buffer[length] = '\0';          
    }
    fclose(file);
    return buffer;
}


static int parse_mtl_map(const char *s, Mtl_map **out, int *consumed) {
    Mtl_map m;
    memset(&m, 0, sizeof(m));
    m.blendu = true;
    m.blendv = true;
    m.s.x = m.s.y = m.s.z = 1.0;

    int i = 0;
    while (s[i] == '-') {
        if (MATCH(s+i, "-blendu ")) {
            i += sizeof("-blendu ") - 1;
            if (MATCH(s+i, "off")) { m.blendu = false; i += 3; }
            else if (MATCH(s+i, "on")) i += 2;
        } else if (MATCH(s+i, "-blendv ")) {
            i += sizeof("-blendv ") - 1;
            if (MATCH(s+i, "off")) { m.blendv = false; i += 3; }
            else if (MATCH(s+i, "on")) i += 2; 
        } else if (MATCH(s+i, "-clamp ")) {
            i += sizeof("-clamp ") - 1;
            if (MATCH(s+i, "on")) { m.clamp = true; i += 2; }
            else if (MATCH(s+i, "off")) i += 3;
        } else if (MATCH(s+i, "-cc ")) {
            i += sizeof("-cc ") - 1;
            if (MATCH(s+i, "on")) { m.cc = true; i += 2; }
            else if (MATCH(s+i, "off")) i += 3;
        } else if (MATCH(s+i, "-bm ")) {
            i += sizeof("-bm ") - 1;
            i += skip_spaces(s+i);
            int c = parse_double(s+i, &m.bm); i += c;
        } else if (MATCH(s+i, "-s ")) {
            i += sizeof("-s ") - 1;
            double nums[4]; int n;
            i += parse_doubles(s+i, nums, &n);
            if (n > 0) m.s.x = nums[0];
            if (n > 1) m.s.y = nums[1];
            if (n > 2) m.s.z = nums[2];
            else m.s.z = 1.0;
        } else if (MATCH(s+i, "-o ")) {
            i += sizeof("-o ") - 1;
            double nums[4]; int n;
            i += parse_doubles(s+i, nums, &n);
            if (n > 0) m.o.x = nums[0];
            if (n > 1) m.o.y = nums[1];
            if (n > 2) m.o.z = nums[2];
        } else if (MATCH(s+i, "-t ")) {
            i += sizeof("-t ") - 1;
            double nums[4]; int n;
            i += parse_doubles(s+i, nums, &n);
            if (n > 0) m.t.x = nums[0];
            if (n > 1) m.t.y = nums[1];
            if (n > 2) m.t.z = nums[2];
        } else if (MATCH(s+i, "-imf ")) {
            i += sizeof("-imf ") - 1;
            m.imf = s[i]; i += 2;
        } else if (MATCH(s+i, "-type ")) {
            i += sizeof("-type ") - 1;
            int start = i;
            while (s[i] && s[i] != ' ' && s[i] != '\n') i++;
            int len = i - start;
            m.type = meta_malloc((size_t)len+1);
            memcpy(m.type, s + start, (size_t)len);
            m.type[len] = '\0';
        } else
            while (s[i] && s[i] != ' ') i++;
        i += skip_spaces(s+i);
    }

    i += skip_spaces(s+i);
    int start = i;
    while (s[i] && s[i] != '\n' && s[i] != '\r') i++;
    int len = i - start;

    m.way = meta_malloc((size_t)len+1);
    memcpy(m.way, s+start, (size_t)len);

    while (len > 0 && m.way[len-1] == ' ') len--;
    m.way[len] = '\0';

    *out = meta_calloc(1, sizeof(Mtl_map));
    **out = m;
    *consumed = i;
    return 1;
}


int Load_Mtl(const char *filename, Parse_OBJ *result) {
    char *data = file_read(filename);
    if (!data) {
        printf("MTL not found: %s\n", filename);
        return 0;
    }


    int cap = (((uint64_t*)result->material)[-1]/sizeof(Mtl)) + 64;
    Mtl *mats = meta_realloc(result->material, (size_t)cap * sizeof(Mtl));
    int cur = result->size_mtl;

    int i = 0;
    while (data[i] != '\0') {
        
        if (data[i] == '\r' || data[i] == '\n' || data[i] == ' ' || data[i] == '\t') {
            i++; continue;
        }

        if (MATCH(data+i, "newmtl ")) {
            i += sizeof("newmtl ") - 1;
           
            if (cur >= cap) {
                cap += 64;
                mats = meta_realloc(mats, (size_t)cap * sizeof(Mtl));
            }
			
            memset(&mats[cur], 0, sizeof(Mtl));
            int len = 0;
            while (data[i+len] && data[i+len] != '\n' && data[i+len] != '\r') len++;
            
			mats[cur].name = meta_malloc((size_t)len + 1);
            memcpy(mats[cur].name, data+i, len);
            mats[cur].name[len] = '\0';
			
            i += len; cur++;
            continue;
        }

        if (cur == 0) { i += skip_line(data+i); i++; continue; } 

        Mtl *m = &mats[cur-1];

        if (MATCH(data+i, "Ns ")) {
            i += sizeof("Ns ") - 1;
            i += skip_spaces(data+i);
            i += parse_double(data+i, &m->Ns);
			
        } else if (MATCH(data+i, "Ni ")) {
            i += sizeof("Ni ") - 1;
            i += skip_spaces(data+i);
            i += parse_double(data+i, &m->Ni);
			
        } else if (MATCH(data+i, "Ka ")) {
            i += sizeof("Ka ") - 1;
            double v[4]; int n;
            i += parse_doubles(data+i, v, &n);
            if (n>0) m->Ka.x=v[0]; if (n>1) m->Ka.y=v[1]; if (n>2) m->Ka.z=v[2];
			
        } else if (MATCH(data+i, "Kd ")) {
            i += sizeof("Kd ") - 1;
            double v[4]; int n;
            i += parse_doubles(data+i, v, &n);
            if (n>0) m->Kd.x=v[0]; if (n>1) m->Kd.y=v[1]; if (n>2) m->Kd.z=v[2];
			
        } else if (MATCH(data+i, "Ks ")) {
            i += sizeof("Ks ") - 1;
            double v[4]; int n;
            i += parse_doubles(data+i, v, &n);
            if (n>0) m->Ks.x=v[0]; if (n>1) m->Ks.y=v[1]; if (n>2) m->Ks.z=v[2];
			
        } else if (MATCH(data+i, "Ke ")) {
            i += sizeof("Ke ") - 1;
            double v[4]; int n;
            i += parse_doubles(data+i, v, &n);
            if (n>0) m->Ke.x=v[0]; if (n>1) m->Ke.y=v[1]; if (n>2) m->Ke.z=v[2];
			
        } else if (MATCH(data+i, "d ")) {
            i += sizeof("d ") - 1;
            i += skip_spaces(data+i);
            i += parse_double(data+i, &m->opacity);
			
        } else if (MATCH(data+i, "Tr ")) {
            i += sizeof("Tr ") - 1;
            i += skip_spaces(data+i);
            double tr; i += parse_double(data+i, &tr);
            m->opacity = 1.0 - tr;
			
        } else if (MATCH(data+i, "illum ")) {
            i += sizeof("illum ") - 1;
            i += skip_spaces(data+i);
            int v; i += parse_int(data+i, &v);
            m->illum = (int8_t)v;
			
        } else if (MATCH(data+i, "map_Kd ")) {
            i += sizeof("map_Kd ") - 1;
            int c; parse_mtl_map(data+i, &m->map_Kd, &c); i += c;
			
        } else if (MATCH(data+i, "map_Ka ")) {
            i += sizeof("map_Ka ") - 1;
            int c; parse_mtl_map(data+i, &m->map_Ka, &c); i += c;
			
        } else if (MATCH(data+i, "map_Ks ")) {
            i += sizeof("map_Ks ") - 1;
            int c; parse_mtl_map(data+i, &m->map_Ks, &c); i += c;
			
        } else if (MATCH(data+i, "map_Ns ")) {
            i += sizeof("map_Ns ") - 1;
            int c; parse_mtl_map(data+i, &m->map_Ns, &c); i += c;
			
        } else if (MATCH(data+i, "map_d ")) {
            i += sizeof("map_d ") - 1;
            int c; parse_mtl_map(data+i, &m->map_opacity, &c); i += c;
			
        } else if (MATCH(data+i, "map_bump ") || MATCH(data+i, "bump ") ||
                   MATCH(data+i, "map_Bump ") || MATCH(data+i, "Bump ")) {
 
            while (data[i] && data[i] != ' ') i++;
            i += skip_spaces(data+i);
            int c; parse_mtl_map(data+i, &m->map_bump, &c); i += c;
			
        } else 
            i += skip_line(data+i);

        while (data[i] && data[i] != '\n') i++;
        if (data[i] == '\n') i++;
    }

    mats = meta_realloc(mats, (size_t)cur * sizeof(Mtl));
    result->material  = mats;
    result->size_mtl  = cur;
    meta_free(data);
    return 1;
}


int Read_Tringles_OBJ(const char *data, Parse_OBJ *obj) {
    int mtl_count = obj->size_mtl;

    Vertex **tringles = meta_calloc((size_t)mtl_count, sizeof(Vertex*));
    int    *tri_count = meta_calloc((size_t)mtl_count, sizeof(int));
    int    *tri_cap   = meta_calloc((size_t)mtl_count, sizeof(int));

    tringles[0] = meta_calloc(4096, sizeof(Vertex));
    tri_cap[0]  = 4096;

    int      vec_cap = 512;
    Vertex  *verts   = meta_calloc((size_t)vec_cap, sizeof(Vertex));

    int usemtl = 0;
    int i = 0;

    while (data[i] != '\0') {

        if (data[i] == '\r' || data[i] == ' ' || data[i] == '\t') { i++; continue; }

        if (data[i] == '\n') { i++; continue; }

        if (MATCH(data+i, "usemtl ")) {
            i += sizeof("usemtl ") - 1;
            
            int start = i;
            while (data[i] && data[i] != '\n' && data[i] != '\r') i++;
            int len = i - start;
            
            while (len > 0 && data[start+len-1] == ' ') len--;

            
            int found = 0;
            for (int m = 0; m < mtl_count; m++) {
                if (obj->material[m].name &&
                    strncmp(data+start, obj->material[m].name, (size_t)len) == 0 &&
                    obj->material[m].name[len] == '\0') {
                    usemtl = m; found = 1; break;
                }
            }
            if (!found) usemtl = 0;

            
            if (tringles[usemtl] == NULL) {
                tringles[usemtl] = meta_calloc(4096, sizeof(Vertex));
                tri_cap[usemtl]  = 4096;
            }
            continue;
        }

        
        if (data[i] == 'f' && (data[i+1] == ' ' || data[i+1] == '\t')) {
            i += 2;
            int nv = 0;

            while (data[i] && data[i] != '\n' && data[i] != '\r') {
                if (data[i] == ' ' || data[i] == '\t') { i++; continue; }

                if (nv >= vec_cap) {
                    vec_cap += 512;
                    verts = meta_realloc(verts, (size_t)vec_cap * sizeof(Vertex));
                }

                Vertex v = {0, 0, 0};

                int val; int c = parse_int(data+i, &val); i += c;
                v.v = (uint32_t)val;

                if (data[i] == '/') {
                    i++;
                    if (data[i] != '/') {
                        c = parse_int(data+i, &val); i += c;
                        v.uv = (uint32_t)val;
                    }
                    if (data[i] == '/') {
                        i++;
                        c = parse_int(data+i, &val); i += c;
                        v.n = (uint32_t)val;
                    }
                }

                verts[nv++] = v;
            }

            if (nv < 3) continue;

            int new_tris = nv - 2;

            if (tri_count[usemtl] + new_tris * 3 > tri_cap[usemtl]) {
                tri_cap[usemtl] = tri_count[usemtl] + new_tris * 3 + 4096;
                tringles[usemtl] = meta_realloc(tringles[usemtl], (size_t)tri_cap[usemtl] * sizeof(Vertex));
            }

			Vertex *dst = &tringles[usemtl][tri_count[usemtl]];
			for (int t = 0; t < new_tris; t++) {
				dst[t*3+0] = verts[0];
				dst[t*3+1] = verts[t+1];
                dst[t*3+2] = verts[t+2];
            }
            tri_count[usemtl] += new_tris * 3;
            continue;
        }

        while (data[i] && data[i] != '\n') i++;
    }

    meta_free(verts);

    for (int m = 0; m < mtl_count; m++) {
        if (tringles[m] == NULL) continue;
        if (tri_count[m] == 0) {
            meta_free(tringles[m]);
            tringles[m] = NULL;
        } else {
            tringles[m] = meta_realloc(tringles[m], (size_t)tri_count[m] * sizeof(Vertex));
        }
    }

    meta_free(tri_count);
    meta_free(tri_cap);
    obj->tringles = tringles;
    return 1;
}

int Read_OBJ(const char *data, Parse_OBJ *result) {
    Parse_OBJ obj;
    memset(&obj, 0, sizeof(Parse_OBJ));

    int v_cap  = 1024, n_cap = 1024, uv_cap = 1024;
    obj.vectors = meta_calloc((size_t)v_cap,  sizeof(Vec4));
    obj.normals = meta_calloc((size_t)n_cap,  sizeof(Normal));
    obj.uvs     = meta_calloc((size_t)uv_cap, sizeof(Uv3));
    obj.coments = meta_calloc(256,            sizeof(char));
    int com_cap = 256;

    int i = 0;
    while (data[i] != '\0') {
        if (data[i] == '\r' || data[i] == ' ' || data[i] == '\t') { i++; continue; }
        if (data[i] == '\n') { i++; continue; }

        if (data[i] == '#') {
            i++;
            if (data[i] == ' ' || data[i] == '\t') i++;
            int start = i;
            while (data[i] && data[i] != '\n' && data[i] != '\r') i++;
            int len = i - start;
          
            if (obj.size_com + len + 2 >= com_cap) {
                com_cap = obj.size_com + len + 256;
                obj.coments = meta_realloc(obj.coments, (size_t)com_cap);
            }
            memcpy(obj.coments + obj.size_com, data + start, (size_t)len);
            obj.size_com += len;
            obj.coments[obj.size_com++] = '\n';
            obj.coments[obj.size_com]   = '\0';
            continue;
        }

        if (MATCH(data+i, "mtllib ")) {
            i += sizeof("mtllib ") - 1;
            int start = i;
            while (data[i] && data[i] != '\n' && data[i] != '\r') i++;
            int len = i - start;
            while (len > 0 && data[start+len-1] == ' ') len--;
            obj.mtllib = meta_malloc((size_t)len + 1);
            memcpy(obj.mtllib, data+start, (size_t)len);
            obj.mtllib[len] = '\0';
            continue;
        }


        if (data[i] == 'o' && (data[i+1] == ' ' || data[i+1] == '\t')) {
            i += 2;
            int start = i;
            while (data[i] && data[i] != '\n' && data[i] != '\r') i++;
            int len = i - start;
            while (len > 0 && data[start+len-1] == ' ') len--;
            obj.obj_name = meta_malloc((size_t)len + 1);
            memcpy(obj.obj_name, data+start, (size_t)len);
            obj.obj_name[len] = '\0';
            continue;
        }

 
        if (data[i] == 's' && (data[i+1] == ' ' || data[i+1] == '\t')) {
            i += 2;
            i += skip_spaces(data+i);
            int v; int c = parse_int(data+i, &v); i += c;
            obj.s = (int8_t)v;
            continue;
        }


        if (data[i] == 'v' && (data[i+1] == ' ' || data[i+1] == '\t')) {
            i += 2;
            if (obj.size_v >= v_cap) {
                v_cap += 1024;
                obj.vectors = meta_realloc(obj.vectors, (size_t)v_cap * sizeof(Vec4));
            }
            Vec4 *v4 = &obj.vectors[obj.size_v];
            v4->x = v4->y = v4->z = 0.0; v4->w = 1.0;
            double nums[4]; int n;
            i += parse_doubles(data+i, nums, &n);
            if (n>0) v4->x=nums[0]; if (n>1) v4->y=nums[1];
            if (n>2) v4->z=nums[2]; if (n>3) v4->w=nums[3];
            obj.size_v++;
            continue;
        }


        if (MATCH(data+i, "vn ") || MATCH(data+i, "vn\t")) {
            i += 3;
            if (obj.size_n >= n_cap) {
                n_cap += 1024;
                obj.normals = meta_realloc(obj.normals, (size_t)n_cap * sizeof(Normal));
            }
            Normal *nm = &obj.normals[obj.size_n];
            double nums[4]; int n;
            i += parse_doubles(data+i, nums, &n);
            nm->x = n>0?nums[0]:0; nm->y = n>1?nums[1]:0; nm->z = n>2?nums[2]:0;
            obj.size_n++;
            continue;
        }


        if (MATCH(data+i, "vt ") || MATCH(data+i, "vt\t")) {
            i += 3;
            if (obj.size_uv >= uv_cap) {
                uv_cap += 1024;
                obj.uvs = meta_realloc(obj.uvs, (size_t)uv_cap * sizeof(Uv3));
            }
            Uv3 *uv = &obj.uvs[obj.size_uv];
            double nums[4]; int n;
            i += parse_doubles(data+i, nums, &n);
            uv->x = n>0?nums[0]:0; uv->y = n>1?nums[1]:0; uv->z = n>2?nums[2]:0;
            obj.size_uv++;
            continue;
        }

        while (data[i] && data[i] != '\n') i++;
    }

    obj.vectors = meta_realloc(obj.vectors, (size_t)obj.size_v  * sizeof(Vec4));
    obj.normals = meta_realloc(obj.normals, (size_t)obj.size_n  * sizeof(Normal));
    obj.uvs     = meta_realloc(obj.uvs,     (size_t)obj.size_uv * sizeof(Uv3));

    obj.material = meta_calloc(1, sizeof(Mtl));
    obj.material[0].name = meta_malloc(8);
    Writing(obj.material[0].name, "Default", 8);
    obj.size_mtl = 1;

    if (obj.mtllib)
        Load_Mtl(obj.mtllib, &obj);

    Read_Tringles_OBJ(data, &obj);

    *result = obj;
    return 1;
}

void free_Mtl_map(Mtl_map *m) {
    if (!m) return;
    meta_free(m->way);
    if (m->type) meta_free(m->type);
    meta_free(m);
}

void free_Mtl_OBJ(Mtl *mats, int count) {
    for (int i = 0; i < count; i++) {
        meta_free(mats[i].name);
        free_Mtl_map(mats[i].map_Ns);
        free_Mtl_map(mats[i].map_Ka);
        free_Mtl_map(mats[i].map_Kd);
        free_Mtl_map(mats[i].map_Ks);
        free_Mtl_map(mats[i].map_opacity);
        free_Mtl_map(mats[i].map_bump);
    }
}

int free_Parse_OBJ(Parse_OBJ *obj) {
    if (!obj) return 0;
    meta_free(obj->vectors);
    meta_free(obj->normals);
    meta_free(obj->uvs);
    meta_free(obj->coments);
    meta_free(obj->mtllib);
    meta_free(obj->obj_name);

    if (obj->tringles) {
        for (int i = 0; i < obj->size_mtl; i++)
            if (obj->tringles[i]) meta_free(obj->tringles[i]);
        meta_free(obj->tringles);
    }

    if (obj->material) {
        free_Mtl_OBJ(obj->material, obj->size_mtl);
        meta_free(obj->material);
    }
    return 1;
}
