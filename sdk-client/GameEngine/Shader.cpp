/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pch.h"
#include <cstdlib>

#ifdef __ANDROID__
#include <GLES2/gl2.h>
#else
#endif

#include "Shader.h"
#include "JNIHelper.h"

namespace ndk_helper {

    bool shader::CompileShader(GLuint *shader, const GLenum type,
                               const GLchar *source, const int32_t iSize) {
        if (source == NULL || iSize <= 0) return false;

        *shader = glCreateShader(type);
        glShaderSource(*shader, 1, &source, &iSize);  // Not specifying 3rd parameter
        // (size) could be troublesome..

        glCompileShader(*shader);

#if defined(_DEBUG)
        GLint logLength;
        glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLchar *log = (GLchar *) malloc(logLength);
            glGetShaderInfoLog(*shader, logLength, &logLength, log);
            LOGI("Shader compile log:\n%s", log);
            free(log);
        }
#endif

        GLint status;
        glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
        if (status == 0) {
            glDeleteShader(*shader);
            return false;
        }

        return true;
    }

    bool shader::CompileShader(GLuint *shader, const GLenum type,
                               std::vector <uint8_t> &data) {
        if (!data.size()) return false;

        const GLchar *source = (GLchar * ) & data[0];
        int32_t iSize = (int32_t) data.size();
        return shader::CompileShader(shader, type, source, iSize);
    }

    bool shader::CompileShader(GLuint *shader, const GLenum type,
                               const char *strFileName) {
        std::vector <uint8_t> data;
        bool b = JNIHelper::GetInstance()->ReadFile(strFileName, &data);
        if (!b) {
            LOGI("Can not open a file:%s", strFileName);
            return false;
        }

        return shader::CompileShader(shader, type, data);
    }

    bool shader::LinkProgram(const GLuint prog) {
        GLint status;

        glLinkProgram(prog);

#if defined(_DEBUG)
        GLint logLength;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLchar *log = (GLchar *) malloc(logLength);
            glGetProgramInfoLog(prog, logLength, &logLength, log);
            LOGI("Program link log:\n%s", log);
            free(log);
        }
#endif

        glGetProgramiv(prog, GL_LINK_STATUS, &status);
        if (status == 0) {
            LOGI("Program link failed\n");
            return false;
        }

        return true;
    }

    bool shader::ValidateProgram(const GLuint prog) {
        GLint logLength, status;

        glValidateProgram(prog);
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLchar *log = (GLchar *) malloc(logLength);
            glGetProgramInfoLog(prog, logLength, &logLength, log);
            LOGI("Program validate log:\n%s", log);
            free(log);
        }

        glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
        if (status == 0) return false;

        return true;
    }

}  // namespace ndkHelper