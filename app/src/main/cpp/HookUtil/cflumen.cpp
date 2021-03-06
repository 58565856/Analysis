/* Copyright (c) 2015, Jorrit 'Chainfire' Jongma
   See LICENSE file for details */

/* How it works:
 * - disable hardware overlay (happens externally)
 * - hijack fragment shaders generated by: http://androidxref.com/4.4_r1/xref/frameworks/native/services/surfaceflinger/RenderEngine/ProgramCache.cpp
 * --- add extra uniform and line to shader source, to multiply with our color matrix
 * --- keep copy for each program (and mappings of shaders and uniforms) in original form for when disabled, and modified form for when enabled
 */

#include <jni.h>
#include <dlfcn.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <mutex>

#define LOG_TAG "CF.lumen:Inject"
#include "ndklog.h"

#include "maps.h"

#include "hook.h"

using namespace std;

#if defined(__aarch64__) || defined(__x86_64__) || defined(__mips64)
// because of failing builds on ndk10-64 otherwise, we load libc
// dynamically and resolve __system_property_get instead of including
// sys/system_properties.h. Problem with libc.a/so in ndk10-64 ?
// OLD: #include "sys/system_properties.h"
#include <dlfcn.h>

#define PROP_NAME_MAX   32
#define PROP_VALUE_MAX  92

static int (*_dl_system_property_get)(const char *name, char *value);
static void* libc = NULL;

int __system_property_get(const char *name, char *value) {
    if (libc == NULL) {
        _dl_system_property_get = NULL;
        libc = dlopen("libc.so", RTLD_NOW);
    }
    if ((libc != NULL) && (_dl_system_property_get == NULL)) {
        _dl_system_property_get = (int (*)(const char *name, char *value))dlsym(libc, "__system_property_get");
    }
    if (_dl_system_property_get != NULL) {
        return _dl_system_property_get(name, value);
    } else {
        return 0;
    }
}
#else
#include <sys/system_properties.h>
#endif

extern "C" {

static char* lastSettings = NULL;
static int colorize = 0;
static GLfloat matrix[16] = { };
static int antiFlicker = 0;

static int currentProgram = 0;
static int translateProgram = 0;
static int currentFramebuffer = 0;

static recursive_mutex rmutex;

static void copyGLfloats(GLfloat* source, GLfloat* dest, int len) {
    for (int i = 0; i < len; i++) {
        dest[i] = source[i];
    }
}

static void setMatrix(GLfloat* values) {
    if (values != NULL) {
#ifdef DEBUG
        for (int i = 0; i < 4; i++) {
            LOGD("matrix[%.3f %.3f %.3f %.3f]", values[(i * 4)], values[(i * 4) + 1], values[(i * 4) + 2], values[(i * 4) + 3]);
        }
#endif
        copyGLfloats(values, matrix, 16);
        colorize = 1;
    } else {
        LOGD("matrix[null]");
        colorize = 0;
    }
}

static int updateSettings() {
    // cflumen.control.v4
    //      missing, "", or "disabled": disable
    //      16 float values separated and trailed by ':': matrix

    char prop[PROP_VALUE_MAX];
    memset(prop, 0, PROP_VALUE_MAX);
    int len = __system_property_get("cflumen.control.v4", prop);

    {
        if (lastSettings != NULL) {
            if (len == 0) {
                free(lastSettings);
                lastSettings = NULL;
            } else {
                if (strcmp(lastSettings, prop) == 0) {
                    return 0;
                }
            }
        }
        if (lastSettings != NULL) {
            free(lastSettings);
        }
        int l = strlen(prop);
        lastSettings = (char*)malloc(l + 1);
        lastSettings[l] = '\0';
        memcpy(&lastSettings[0], &prop[0], l);
    }

    if (strncmp(prop, "antiflicker", 11) == 0) {
        LOGD("updateSettings: antiflicker");
        setMatrix(NULL);
        antiFlicker = 1;
        return 1;
    }

    if ((len == 0) || (strcmp(prop, "disabled") == 0)) {
        LOGD("updateSettings: disabled");
        setMatrix(NULL);
        antiFlicker = 0;
        return 1;
    }

    char* parts[16] = { };
    int l = strlen(prop) + 1;
    int cur = 0;
    int start = 0;
    for (int i = 0; i < l; i++) {
        if ((prop[i] == ':') || (prop[i] == '\0')) {
            prop[i] = '\0';
            parts[cur] = &prop[start];
            start = i + 1;
            cur++;
            if (cur == 16) {
                LOGD("updateSettings: setting matrix");
                GLfloat matrix[16] = {
                    (float)atof(parts[ 0]), (float)atof(parts[ 1]), (float)atof(parts[ 2]), (float)atof(parts[ 3]),
                    (float)atof(parts[ 4]), (float)atof(parts[ 5]), (float)atof(parts[ 6]), (float)atof(parts[ 7]),
                    (float)atof(parts[ 8]), (float)atof(parts[ 9]), (float)atof(parts[10]), (float)atof(parts[11]),
                    (float)atof(parts[12]), (float)atof(parts[13]), (float)atof(parts[14]), (float)atof(parts[15])
                };
                setMatrix(matrix);
                antiFlicker = 0;
                return 1;
            }
        }
    }

    LOGD("updateSettings: catch-all, cur[%d]", cur);
    setMatrix(NULL);
    antiFlicker = 0;
    return 1;
}

static int setupDone = 0;
static void setup() {
    if (!setupDone) {
        mapsInit();
        setbuf(stdout, NULL);
        updateSettings();
        setupDone = 1;
    }
}

static void glError(const char* func) {
    GLenum error = glGetError();
    if (error != 0) {
        LOGD("ERROR: %s --> 0x%04x", func, error);
    }
}

DEFINEHOOK(GLint, glGetUniformLocation, (GLuint program, const GLchar* name)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glGetUniformLocation(%d, %s)", program, name);
    GLint ret = ORIGINAL(glGetUniformLocation, program, name);
    GLint p = getProgram(program);
    if (p != -1) {
        GLint rep = ORIGINAL(glGetUniformLocation, p, name);
        addUniformLocation(program, ret, rep);
    }
    return ret;
}

#ifdef DEBUG
static int getUniformName(GLuint program, GLuint index, char* buf, int bufSize) {
    memset(buf, 0, bufSize);
    GLsizei written = 0;
    GLint unisize;
    GLenum unitype;
    glGetActiveUniform(program, index, bufSize, &written, &unisize, &unitype, (GLchar*)buf);
    return written;
}
#endif

static GLint translateLocation(GLint location) {
    lock_guard<recursive_mutex> lock(rmutex);

    if (colorize && (currentFramebuffer == 0)) {
        int ret = getUniformLocation(currentProgram, location);
#ifdef DEBUG
        char nameOrg[32];
        char nameRep[32];
        getUniformName(currentProgram, location, nameOrg, 32);
        getUniformName(translateProgram, ret, nameRep, 32);
        LOGD("uniformName(%d,%d)-->%s|%s", currentProgram, location, nameOrg, nameRep);
#endif
        return ret;
    } else {
#ifdef DEBUG
        char name[32];
        getUniformName(currentProgram, location, name, 32);
        LOGD("uniformName(%d,%d)-->%s", currentProgram, location, name);
#endif
        return location;
    }
}

DEFINEHOOK(void, glUniform1i, (GLint location, GLint x)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    GLint translation = translateLocation(location);
    LOGD("glUniform1i(%d-->%d)", location, translation);
    ORIGINAL(glUniform1i, translation > -1 ? translation : location, x);
    glError("glUniform1i");
}

DEFINEHOOK(void, glUniform1f, (GLint location, GLfloat x)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    GLint translation = translateLocation(location);
    LOGD("glUniform1f(%d-->%d)", location, translation);
    ORIGINAL(glUniform1f, translation > -1 ? translation : location, x);
    glError("glUniform1f");
}

DEFINEHOOK(void, glUniform1fv, (GLint location, GLsizei count, const GLfloat* v)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    GLint translation = translateLocation(location);
    LOGD("glUniform1fv(%d-->%d)", location, translation);
    ORIGINAL(glUniform1fv, translation > -1 ? translation : location, count, v);
    glError("glUniform1fv");
}

DEFINEHOOK(void, glUniform4f, (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    GLint translation = translateLocation(location);
    LOGD("glUniform4f(%d-->%d)", location, translation);
    ORIGINAL(glUniform4f, translation > -1 ? translation : location, x, y, z, w);
    glError("glUniform4f");
}

DEFINEHOOK(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat* v)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    GLint translation = translateLocation(location);
    LOGD("glUniform4fv(%d-->%d)", location, translation);
    ORIGINAL(glUniform4fv, translation > -1 ? translation : location, count, v);
    glError("glUniform4fv");
}

DEFINEHOOK(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    GLint translation = translateLocation(location);
    LOGD("glUniformMatrix4fv(%d-->%d)", location, translation);
    /*
    LOGD("Matrix4 %.5f %.5f %.5f %.5f", value[ 0], value[ 1], value[ 2], value[ 3]);
    LOGD("Matrix4 %.5f %.5f %.5f %.5f", value[ 4], value[ 5], value[ 6], value[ 7]);
    LOGD("Matrix4 %.5f %.5f %.5f %.5f", value[ 8], value[ 9], value[10], value[11]);
    LOGD("Matrix4 %.5f %.5f %.5f %.5f", value[12], value[13], value[14], value[15]);
    */
    ORIGINAL(glUniformMatrix4fv, translation > -1 ? translation : location, count, transpose, value);
    glError("glUniformMatrix4fv");
}

DEFINEHOOK(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glBindAttribLocation(%d, %d, %s)", program, index, name);
    ORIGINAL(glBindAttribLocation, program, index, name);
    GLint p = getProgram(program);
    if (p != -1) {
        ORIGINAL(glBindAttribLocation, p, index, name);
    }
}

DEFINEHOOK(GLuint, glCreateShader, (GLenum type)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glCreateShader(%d)", type);
    GLuint ret = ORIGINAL(glCreateShader, type);
    if (type == GL_VERTEX_SHADER) {
        addShader(ret, ret);
    } else if (type == GL_FRAGMENT_SHADER) {
        addShader(ret, ORIGINAL(glCreateShader, type));
    }
    return ret;
}

#if defined(__aarch64__) || defined(__x86_64__) || defined(__mips64)
typedef void (*glShaderSource2_t)(GLuint shader, GLsizei count, const GLchar* const* source, const GLint* length);
#else
typedef void (*glShaderSource2_t)(GLuint shader, GLsizei count, const GLchar** source, const GLint* length);
#endif
glShaderSource2_t original_glShaderSource2;

// http://androidxref.com/5.0.0_r2/xref/frameworks/native/services/surfaceflinger/RenderEngine/ProgramCache.h
#define BLEND_PREMULT           0x00000001
#define BLEND_NORMAL            0x00000000
#define OPACITY_OPAQUE          0x00000002
#define OPACITY_TRANSLUCENT     0x00000000
#define PLANE_ALPHA_LT_ONE      0x00000004
#define PLANE_ALPHA_EQ_ONE      0x00000000
#define TEXTURE_OFF             0x00000000
#define TEXTURE_EXT             0x00000008
#define TEXTURE_2D              0x00000010
#define COLOR_MATRIX_OFF        0x00000000
#define COLOR_MATRIX_ON         0x00000020

#if defined(__aarch64__) || defined(__x86_64__) || defined(__mips64)
static void glShaderSourceCopy(GLuint shader, GLsizei count, const GLchar* const* source, const GLint* length) {
#else
static void glShaderSourceCopy(GLuint shader, GLsizei count, const GLchar** source, const GLint* length) {
#endif
    LOGD("glShaderSourceCopy(%d, %d, ..., %d)", shader, count, length != NULL ? *length : 0);

    vector<string*> lines;
    for (int i = 0; i < count; i++) {
        int l = -1;
        if (length != NULL) {
            l = length[i];
        }
        if (l < 0) {
            l = strlen(source[i]);
        }

        char* line = (char*)malloc(l + 1);
        memset(line, 0, l + 1);
        memcpy(line, source[i], l);

        int start = 0;
        for (int i = 0; i < l; i++) {
            if (line[i] == '\n') {
                if (i > start) {
                    lines.push_back(new string(&line[start], i - start));
                }
                start = i + 1;
            }
        }
        if (start < l) {
            lines.push_back(new string(&line[start], l - start));
        }

        free(line);
    }
    for (int i = 0; i < (int)lines.size(); i++) {
        LOGD("SRC#%d: %s", i + 1, lines.at(i)->c_str());
    }

    GLint s = getShader(shader);
    if ((s != -1) && (s != (GLint)shader)) {
        LOGD("%d-->%d", shader, s);

        // Android currently passes the shaders as a single null terminated source
        if (count == 1) {
            // http://androidxref.com/5.0.0_r2/xref/frameworks/native/services/surfaceflinger/RenderEngine/ProgramCache.cpp
            // http://androidxref.com/6.0.1_r10/xref/frameworks/native/services/surfaceflinger/RenderEngine/ProgramCache.cpp

            int texture = TEXTURE_OFF;
            int planeAlpha = PLANE_ALPHA_EQ_ONE;
            int colorMatrix = COLOR_MATRIX_OFF;
            int opacity = OPACITY_TRANSLUCENT;

            int blend = BLEND_NORMAL;
            GLboolean blending;
            glGetBooleanv(GL_BLEND, &blending);
            if (blending == GL_TRUE) {
                GLint blendSource;
                glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSource);
                if (blendSource == GL_ONE) {
                    blend = BLEND_PREMULT;
                }
            }

            int fragmentShader = 0;

            for (auto it = lines.begin(); it != lines.end(); it++) {
                if (
                    ((*it)->find("gl_FragColor") != string::npos)
                ) {
                    fragmentShader = 1;
                }

                if (
                    ((*it)->find("#extension GL_OES_EGL_image_external : require") != string::npos) ||
                    ((*it)->find("uniform samplerExternalOES sampler;") != string::npos)
                ) {
                    texture = TEXTURE_EXT;
                }

                if (
                    ((*it)->find("uniform sampler2D sampler;") != string::npos)
                ) {
                    texture = TEXTURE_2D;
                }

                if (
                    ((*it)->find("uniform float alphaPlane;") != string::npos)
                ) {
                    planeAlpha = PLANE_ALPHA_LT_ONE;
                }

                if (
                    ((*it)->find("uniform mat4 colorMatrix;") != string::npos)
                ) {
                    colorMatrix = COLOR_MATRIX_ON;
                }

                if (
                    ((*it)->find("gl_FragColor.a = 1.0;") != string::npos)
                ) {
                    opacity = OPACITY_OPAQUE;
                }
            }

            if (fragmentShader) {
                // swallow unused variable errors in non-debug mode
                (void)texture;
                (void)planeAlpha;

                LOGD("texture[%s] planeAlpha[%d] colorMatrix[%d] opaque[%d] premult[%d]",
                    (texture == TEXTURE_EXT) ? "EXT" : ((texture == TEXTURE_2D) ? "2D" : "OFF"),
                    planeAlpha == PLANE_ALPHA_LT_ONE ? 1 : 0,
                    colorMatrix == COLOR_MATRIX_ON ? 1 : 0,
                    opacity == OPACITY_OPAQUE ? 1 : 0,
                    blend == BLEND_PREMULT ? 1 : 0
                );

                if ((colorMatrix == COLOR_MATRIX_OFF)) {
                    for (auto it = lines.begin(); it != lines.end(); it++) {
                        if ((*it)->find("void main(void) {") != string::npos) {
                            lines.insert(it, new string("uniform mat4 colorMatrix;"));
                            break;
                        }
                    }
                    for (auto it = lines.begin(); it != lines.end(); it++) {
                        if ((*it)->find("}") != string::npos) {
                            vector<string*> insert;
                            if ((opacity != OPACITY_OPAQUE) && (blend == BLEND_PREMULT)) {
                                insert.push_back(new string("    gl_FragColor.rgb = gl_FragColor.rgb/gl_FragColor.a;"));
                            }
                            insert.push_back(new string("    vec4 transformed = colorMatrix*vec4(gl_FragColor.rgb, 1);"));
                            insert.push_back(new string("    gl_FragColor.rgb = transformed.rgb/transformed.a;"));
                            if ((opacity != OPACITY_OPAQUE) && (blend == BLEND_PREMULT)) {
                                insert.push_back(new string("    gl_FragColor.rgb = gl_FragColor.rgb*gl_FragColor.a;"));
                            }
                            lines.insert(it, insert.begin(), insert.end());
                            break;
                        }
                    }
                }
            }

            int l = lines.size();
            for (auto it = lines.begin(); it != lines.end(); it++) {
                l += (*it)->length();
            }
            char* sourceMod = (char*)malloc(l + 1);
            memset(sourceMod, 0, l + 1);

            int p = 0;
            for (auto it = lines.begin(); it != lines.end(); it++) {
                memcpy(&sourceMod[p], (*it)->c_str(), (*it)->length());
                p += (*it)->length();
                sourceMod[p] = '\n';
                p++;
            }

            for (int i = 0; i < (int)lines.size(); i++) {
                LOGD("DST#%d: %s", i + 1, lines.at(i)->c_str());
            }

            const GLchar* list[1] = { (GLchar*)sourceMod };
            original_glShaderSource2(s, 1, list, NULL);
            glError("glShaderSource");

            free(sourceMod);
        }
    }

    for (int i = 0; i < (int)lines.size(); i++) {
        delete(lines.at(i));
    }
}

#if defined(__aarch64__) || defined(__x86_64__) || defined(__mips64)
DEFINEHOOK(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar* const* source, const GLint* length)) {
#else
DEFINEHOOK(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar** source, const GLint* length)) {
#endif
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    original_glShaderSource2 = original_glShaderSource;
    glShaderSourceCopy(shader, count, source, length);
    ORIGINAL(glShaderSource, shader, count, source, length);
}

DEFINEHOOK(void, glCompileShader, (GLuint shader)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glCompileShader(%d)", shader);
    ORIGINAL(glCompileShader, shader);
    GLint s = getShader(shader);
    if ((s != -1) && (s != (GLint)shader)) {
        LOGD("+glCompileShader(%d)", s);
        ORIGINAL(glCompileShader, s);
        glError("glCompileShader");
    }
}

DEFINEHOOK(GLuint, glCreateProgram, ()) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glCreateProgram()");
    GLuint ret = ORIGINAL(glCreateProgram);
    addProgram(ret, ORIGINAL(glCreateProgram));
    return ret;
}

DEFINEHOOK(void, glAttachShader, (GLuint program, GLuint shader)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glAttachShader(%d, %d)", program, shader);
    ORIGINAL(glAttachShader, program, shader);
    GLint p = getProgram(program);
    if (p != -1) {
        GLint s = getShader(shader);
        if (s != -1) {
            LOGD("+glAttachShader(%d, %d)", p, s);
            ORIGINAL(glAttachShader, p, s);
            glError("glAttachShader");
        }
    }
}

DEFINEHOOK(void, glLinkProgram, (GLuint program)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glLinkProgram(%d)", program);
    ORIGINAL(glLinkProgram, program);
    GLint p = getProgram(program);
    if (p != -1) {
        LOGD("+glLinkProgram(%d)", p);
        ORIGINAL(glLinkProgram, p);
        glError("glLinkProgram");
    }
}

typedef void (*glUseProgram2_t)(GLuint program);
glUseProgram2_t original_glUseProgram2;

static void reverseProgram(GLuint program) {
    LOGD("reverseProgram(%d)", program);
    GLuint p = ORIGINAL(glCreateProgram);
    glError("glCreateProgram");

    // Shaders
    GLuint shaders[16];
    GLsizei shaderCount = 0;
    glGetAttachedShaders(program, 16, &shaderCount, shaders);
    glError("glGetAttachedShaders");
    for (int i = 0; i < shaderCount; i++) {
        LOGD("reverseProgram::shader(%d/%d)", i + 1, shaderCount);
        GLint shaderType = 0;
        GLint shaderLength = 0;
        glGetShaderiv(shaders[i], GL_SHADER_TYPE, &shaderType);
        glError("glGetShaderiv");
        glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &shaderLength);
        glError("glGetShaderiv");
        if ((shaderType > 0) && (shaderLength > 0)) {
            LOGD("reverseProgram::shader(type:%d/length:%d)", shaderType, shaderLength);
            char* source = (char*)malloc(shaderLength + 1);
            GLsizei returnedShaderLength = 0;
            glGetShaderSource(shaders[i], shaderLength + 1, &returnedShaderLength, source);
            glError("glGetShaderSource");
            if (returnedShaderLength > 0) {
                GLint s = ORIGINAL(glCreateShader, shaderType);
                glError("glCreateShader");
                addShader(shaders[i], s);
                original_glShaderSource2 = original_glShaderSource;
                glShaderSourceCopy(shaders[i], 1, (const char**)&source, &returnedShaderLength);
                glCompileShader(s);
                glError("glCompileShader");
                glAttachShader(p, s);
                glError("glAttachShader");
            }
            free(source);
        }
    }

    // Attributes
    GLint attributeCount = 0;
    GLint attributeLengthMax = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attributeCount);
    glError("glGetProgramiv");
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attributeLengthMax);
    glError("glGetProgramiv");
    if ((attributeCount > 0) && (attributeLengthMax > 0)) {
        char* name = (char*)malloc(attributeLengthMax + 1);
        for (int i = 0; i < attributeCount; i++) {
            GLsizei length;
            GLint size;
            GLenum type;
            glGetActiveAttrib(program, i, attributeLengthMax, &length, &size, &type, name);
            LOGD("reverseProgram::attribute(%d/%d/%s)", i + 1, attributeCount, name);
            glError("glGetActiveAttrib");
            ORIGINAL(glBindAttribLocation, p, i, name);
            glError("glBindAttribLocation");
        }
        free(name);
    }

    // Link program
    ORIGINAL(glLinkProgram, p);
    glError("glLinkProgram");

    // Use program, needed to copy uniforms
    GLint oldProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    original_glUseProgram2(p);

    // Uniforms
    GLint uniformCount = 0;
    GLint uniformLengthMax = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
    glError("glGetProgramiv");
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformLengthMax);
    glError("glGetProgramiv");
    if ((uniformCount > 0) && (uniformLengthMax > 0)) {
        char* name = (char*)malloc(uniformLengthMax + 1);
        for (int i = 0; i < uniformCount; i++) {
            GLsizei length;
            GLint size;
            GLenum type;
            glGetActiveUniform(program, i, uniformLengthMax, &length, &size, &type, name);

#ifdef DEBUG
            const char* typeName = "?";
            switch (type) {
            case GL_FLOAT		: typeName = "F1"; break;
            case GL_FLOAT_VEC2	: typeName = "F2"; break;
            case GL_FLOAT_VEC3	: typeName = "F3"; break;
            case GL_FLOAT_VEC4	: typeName = "F4"; break;
            case GL_INT			: typeName = "I1"; break;
            case GL_INT_VEC2	: typeName = "I2"; break;
            case GL_INT_VEC3	: typeName = "I3"; break;
            case GL_INT_VEC4	: typeName = "I4"; break;
            case GL_BOOL		: typeName = "B1"; break;
            case GL_BOOL_VEC2	: typeName = "B2"; break;
            case GL_BOOL_VEC3	: typeName = "B3"; break;
            case GL_BOOL_VEC4	: typeName = "B4"; break;
            case GL_FLOAT_MAT2	: typeName = "M2"; break;
            case GL_FLOAT_MAT3	: typeName = "M3"; break;
            case GL_FLOAT_MAT4	: typeName = "M4"; break;
            case GL_SAMPLER_2D	: typeName = "S2"; break;
            case GL_SAMPLER_CUBE: typeName = "SC"; break;
            }

            LOGD("reverseProgram::uniform(%d/%d/%s/%s/%d)", i + 1, uniformCount, name, typeName, size);
#endif

            glError("glGetActiveUniform");
            GLint l = ORIGINAL(glGetUniformLocation, p, name);
            glError("glGetUniformLocation");
            addUniformLocation(program, i, l);

            if (type == GL_INT) {
                GLint i_;
                glGetUniformiv(program, i, &i_);
                ORIGINAL(glUniform1i, l, i_);
            } else if (type == GL_FLOAT) {
                GLfloat v;
                glGetUniformfv(program, i, &v);
                ORIGINAL(glUniform1f, l, v);
            } else if (type == GL_FLOAT_VEC4) {
                GLfloat v[4];
                glGetUniformfv(program, i, &v[0]);
                ORIGINAL(glUniform4fv, l, 1, (const GLfloat*)&v);
            } else if (type == GL_FLOAT_MAT4) {
                GLfloat v[16];
                glGetUniformfv(program, i, &v[0]);
                ORIGINAL(glUniformMatrix4fv, l, 1, GL_FALSE, (const GLfloat*)&v[0]);
            }
        }
        free(name);
    }

    // Return to previous program
    original_glUseProgram2(oldProgram);

    // Done
    addProgram(program, p);
}

DEFINEHOOK(void, glUseProgram, (GLuint program)) {
    lock_guard<recursive_mutex> lock(rmutex);

    setup();
    LOGD("glUseProgram(%d)", program);
    currentProgram = program;
    translateProgram = program;
    int done = 0;
    if (colorize && (currentFramebuffer == 0)) {
        GLint p = getProgram(program);
        if (p == -1) {
            original_glUseProgram2 = original_glUseProgram;
            reverseProgram(program);
            p = getProgram(program);
        }
        if (p != -1) {
            translateProgram = p;
            int indexColorMatrix = ORIGINAL(glGetUniformLocation, p, "colorMatrix");
            LOGD("+glUseProgram(%d-->%d)(%d)", program, p, indexColorMatrix);
            ORIGINAL(glUseProgram, p);
            if (indexColorMatrix >= 0) {
                ORIGINAL(glUniformMatrix4fv, indexColorMatrix, 1, GL_FALSE, matrix);
            }
            done = 1;
        }
    }
    if (!done) {
        ORIGINAL(glUseProgram, program);
    }
}

DEFINEHOOK(void, glBindFramebuffer, (GLenum target, GLuint framebuffer)) {
    lock_guard<recursive_mutex> lock(rmutex);

    LOGD("glBindFramebuffer(%d, %d)", target, framebuffer);
    if (target == GL_FRAMEBUFFER) currentFramebuffer = framebuffer;
    return ORIGINAL(glBindFramebuffer, target, framebuffer);
}

} // extern "C"

#define NO_ERROR 0
#define PERMISSION_DENIED -EPERM
#define UNKNOWN_TRANSACTION -EBADMSG
DEFINEHOOKPP(int32_t, onTransact, (uint32_t code, void* data, void* reply, uint32_t flags)) {
    int32_t ret = ORIGINALPP(code, data, reply, flags);
    if ((ret == UNKNOWN_TRANSACTION) || (ret == PERMISSION_DENIED)) {
        lock_guard<recursive_mutex> lock(rmutex);

        if (
            (code == 1008) || // disable HWC
            (code == 1014) || // daltonize
            (code == 1015)    // color matrix
        ) {
            setup();
            int updated = updateSettings();
            if (!colorize && !antiFlicker) {
                LOGD("onTransact[%d] passthrough (disabled)", code);
            } else if (colorize && updated && (code == 1008)) {
                LOGD("onTransact[%d] passthrough (colorize)", code);
            } else if (antiFlicker && updated && (code == 1015)) {
                LOGD("onTransact[%d] passthrough (antiFlicker)", code);
            } else {
                LOGD("onTransact[%d] swallow colorize:[%d] antiFlicker[%d]", code, colorize, antiFlicker);
                ret = NO_ERROR;
            }
        }
    }
    return ret;
}

extern "C" {

void hook() {
    LOGD("LOADED INTO %d", getpid());

    REGISTERHOOK(glGetUniformLocation);
    REGISTERHOOK(glUniform1i);
    REGISTERHOOK(glUniform1f);
    REGISTERHOOK(glUniform1fv);
    REGISTERHOOK(glUniform4f);
    REGISTERHOOK(glUniform4fv);
    REGISTERHOOK(glUniformMatrix4fv);
    REGISTERHOOK(glCreateShader);
    REGISTERHOOK(glShaderSource);
    REGISTERHOOK(glCompileShader);
    REGISTERHOOK(glCreateProgram);
    REGISTERHOOK(glAttachShader);
    REGISTERHOOK(glLinkProgram);
    REGISTERHOOK(glUseProgram);
    REGISTERHOOK(glBindAttribLocation);
    REGISTERHOOK(glBindFramebuffer);

    REGISTERHOOKPP("_ZN7android17BnSurfaceComposer10onTransactEjRKNS_6ParcelEPS1_j", onTransact);

    libhook_log(LOG_TAG);
    libhook_hook(1, 1);

    setup();
    updateSettings();
}

}