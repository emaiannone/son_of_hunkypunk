#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <setjmp.h>
#include "glk.h"

JavaVM *_jvm;
jclass _class, _Event, _LineInputEvent, _Window, _FileRef, _Stream;
JNIEnv *_env;
jobject _this;
char *_line_event_buf;
glui32 _line_event_buf_len;
jmp_buf _quit_env;

#define GLK_JNI_VERSION JNI_VERSION_1_2

jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	_jvm = jvm;

	JNIEnv *env;
	if ((*jvm)->GetEnv(jvm, (void **)&env, GLK_JNI_VERSION))
		return JNI_ERR;

	jclass cls = (*env)->FindClass(env, "org/andglk/Glk");
	_class = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "org/andglk/Event");
	_Event = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "org/andglk/LineInputEvent");
	_LineInputEvent = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "org/andglk/Window");
	_Window = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "org/andglk/FileRef");
	_FileRef = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "org/andglk/FileStream");
	_Stream = (*env)->NewGlobalRef(env, cls);

	return GLK_JNI_VERSION;
}

void Java_org_andglk_Glk_runProgram(JNIEnv *env, jobject this)
{
	_this = (*env)->NewGlobalRef(env, this);
	if (!setjmp(_quit_env))
		glk_main();
}

jint Java_org_andglk_CPointed_makePoint(JNIEnv *env, jobject this)
{
	jobject *ptr = malloc(sizeof(jobject));
	*ptr = (*env)->NewGlobalRef(env, this);
	return (jint) ptr;
}

void Java_org_andglk_CPointed_releasePoint(JNIEnv *env, jobject this, jint point)
{
	if (!point)
		return;

	jobject *ptr = (jobject *) point;
	(*env)->DeleteGlobalRef(env, *ptr);
	free(ptr);
}

JNIEnv *JNU_GetEnv()
{
    JNIEnv *env;
    (*_jvm)->GetEnv(_jvm,
                          (void **)&env,
                          JNI_VERSION_1_2);
    return env;
}

void glk_exit(void)
{
	// TODO: cleanup objects

	// any cleaner way to have glk_exit() not returning (as per spec)?
	longjmp(_quit_env, 1);
}

void glk_set_interrupt_handler(void (*func)(void))
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "set_interrupt_handler", "(L)V");

	(*env)->CallVoidMethod(env, _this, mid, func);

}

void glk_tick(void)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "tick", "()V");

	(*env)->CallVoidMethod(env, _this, mid);

}

glui32 glk_gestalt(glui32 sel, glui32 val)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "gestalt", "(JJ)J");

	return (*env)->CallLongMethod(env, _this, mid, sel, val);

}

glui32 glk_gestalt_ext(glui32 sel, glui32 val, glui32 *arr,
    glui32 arrlen)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "gestalt_ext", "(JJ[JJ)J");

	// FIXME: array translation
	return (*env)->CallLongMethod(env, _this, mid, sel, val, arr, arrlen);

}

unsigned char glk_char_to_lower(unsigned char ch)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "char_to_lower", "(C)C");

	return (*env)->CallCharMethod(env, _this, mid, (jchar) ch);

}

unsigned char glk_char_to_upper(unsigned char ch)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "char_to_upper", "(C)C");

	return (*env)->CallCharMethod(env, _this, mid, ch);

}

winid_t glk_window_get_root(void)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_root", "()Lorg/andglk/Window;");

	return (*env)->CallObjectMethod(env, _this, mid);

}

winid_t glk_window_open(winid_t split, glui32 method, glui32 size,
    glui32 wintype, glui32 rock)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_open", "(Lorg/andglk/Window;JJJI)I");

	return (winid_t) (*env)->CallIntMethod(env, _this, mid, split ? *split : 0, (jlong) method, (jlong) size, (jlong) wintype, (jint) rock);
}

void glk_window_close(winid_t win, stream_result_t *result)
{
	if (!win)
		return;

	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _Window, "close", "()J");

	glui32 written = (*env)->CallLongMethod(env, *win, mid);
	if (result) {
		result->readcount = 0;
		result->writecount = written;
	}
}

void glk_window_get_size(winid_t win, glui32 *widthptr, glui32 *heightptr)
{
	if (!win)
		return;

	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _Window, "get_size", "()[J");

	jlongArray res = (*env)->CallObjectMethod(env, *win, mid);
	jlong *arr = (*env)->GetLongArrayElements(env, res, NULL);
	*widthptr = arr[0];
	*heightptr = arr[1];
	(*env)->ReleaseLongArrayElements(env, res, arr, JNI_ABORT);
}

void glk_window_set_arrangement(winid_t win, glui32 method,
    glui32 size, winid_t keywin)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_set_arrangement", "()V");

	(*env)->CallVoidMethod(env, _this, mid, win, method, size, keywin);

}

void glk_window_get_arrangement(winid_t win, glui32 *methodptr,
    glui32 *sizeptr, winid_t *keywinptr)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_arrangement", "(Lorg/andglk/Window;)V");

	(*env)->CallVoidMethod(env, _this, mid, win, methodptr, sizeptr, keywinptr);

}

winid_t glk_window_iterate(winid_t win, glui32 *rockptr)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_iterate", "(Lorg/andglk/Window;)Lorg/andglk/Window;");

	return (*env)->CallObjectMethod(env, _this, mid, win, rockptr);

}

glui32 glk_window_get_rock(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_rock", "(Lorg/andglk/Window;)J");

	return (*env)->CallLongMethod(env, _this, mid, win);

}

glui32 glk_window_get_type(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_type", "(Lorg/andglk/Window;)J");

	return (*env)->CallLongMethod(env, _this, mid, win);

}

winid_t glk_window_get_parent(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_parent", "(Lorg/andglk/Window;)Lorg/andglk/Window;");

	return (*env)->CallObjectMethod(env, _this, mid, win);

}

winid_t glk_window_get_sibling(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_sibling", "(Lorg/andglk/Window;)Lorg/andglk/Window;");

	return (*env)->CallObjectMethod(env, _this, mid, win);

}

void glk_window_clear(winid_t win)
{
	if (!win)
		return;

	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _Window, "clear", "()V");

	(*env)->CallVoidMethod(env, *win, mid);
}

void glk_window_move_cursor(winid_t win, glui32 xpos, glui32 ypos)
{
	if (!win)
		return;

	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _Window, "move_cursor", "(JJ)V");

	(*env)->CallVoidMethod(env, *win, mid, (jlong) xpos, (jlong)ypos);
}

strid_t glk_window_get_stream(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_stream", "(Lorg/andglk/Window;)Lorg/andglk/Stream;");

	return (*env)->CallObjectMethod(env, _this, mid, win);

}

void glk_window_set_echo_stream(winid_t win, strid_t str)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_set_echo_stream", "(Lorg/andglk/Window;Lorg/andglk/Stream;)V");

	(*env)->CallVoidMethod(env, _this, mid, win, str);

}

strid_t glk_window_get_echo_stream(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "window_get_echo_stream", "(Lorg/andglk/Window;)Lorg/andglk/Stream;");

	return (*env)->CallObjectMethod(env, _this, mid, win);

}

void glk_set_window(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "set_window", "(Lorg/andglk/Window;)V");

	(*env)->CallVoidMethod(env, _this, mid, win ? *win : NULL);
}

strid_t glk_stream_open_file(frefid_t fileref, glui32 fmode, glui32 rock)
{
	if (!fileref)
		return;

	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetStaticMethodID(env, _Stream, "openFile", "(Lorg/andglk/FileRef;II)I");

	return (strid_t) (*env)->CallStaticIntMethod(env, _Stream, mid, *fileref, (jint) fmode, (jint) rock);
}

strid_t glk_stream_open_memory(char *buf, glui32 buflen, glui32 fmode,
    glui32 rock)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_open_memory", "(JJJ)Lorg/andglk/Stream;");

	return (*env)->CallObjectMethod(env, _this, mid, buf, buflen, fmode, rock);

}

void glk_stream_close(strid_t str, stream_result_t *result)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_close", "(Lorg/andglk/Stream;[FIXME: stream_result_t *])V");

	(*env)->CallVoidMethod(env, _this, mid, str, result);

}

strid_t glk_stream_iterate(strid_t str, glui32 *rockptr)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_iterate", "(Lorg/andglk/Stream;[FIXME: glui32 *])Lorg/andglk/Stream;");

	return (*env)->CallObjectMethod(env, _this, mid, str, rockptr);

}

glui32 glk_stream_get_rock(strid_t str)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_get_rock", "(Lorg/andglk/Stream;)J");

	return (*env)->CallLongMethod(env, _this, mid, str);

}

void glk_stream_set_position(strid_t str, glsi32 pos, glui32 seekmode)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_set_position", "(Lorg/andglk/Stream;IJ)V");

	(*env)->CallVoidMethod(env, _this, mid, str, pos, seekmode);

}

glui32 glk_stream_get_position(strid_t str)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_get_position", "(Lorg/andglk/Stream;)J");

	return (*env)->CallLongMethod(env, _this, mid, str);

}

void glk_stream_set_current(strid_t str)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_set_current", "(Lorg/andglk/Stream;)V");

	(*env)->CallVoidMethod(env, _this, mid, str);

}

strid_t glk_stream_get_current(void)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stream_get_current", "()Lorg/andglk/Stream;");

	return (*env)->CallObjectMethod(env, _this, mid);

}

void glk_put_char(unsigned char ch)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "put_char", "(C)V");

	(*env)->CallVoidMethod(env, _this, mid, (jchar) ch);

}

void glk_put_char_stream(strid_t str, unsigned char ch)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "put_char_stream", "(Lorg/andglk/Stream;C)V");

	(*env)->CallVoidMethod(env, _this, mid, str, ch);

}

void glk_put_string(char *s)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "put_string", "(Ljava/lang/String;)V");

	jchar buf[1024];
	char *it = s;
	jchar *jt = buf;
	while (jt - buf < 1024 && *it)
		*(jt++) = *(it++);

	jstring str = (*env)->NewString(env, buf, it - s);

	(*env)->CallVoidMethod(env, _this, mid, str);
}

void glk_put_string_stream(strid_t str, char *s)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "put_string_stream", "(Lorg/andglk/Stream;[FIXME: char *])V");

	(*env)->CallVoidMethod(env, _this, mid, str, s);

}

void glk_put_buffer(char *buf, glui32 len)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "put_buffer", "([FIXME: char *]J)V");

	(*env)->CallVoidMethod(env, _this, mid, buf, len);

}

void glk_put_buffer_stream(strid_t str, char *buf, glui32 len)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "put_buffer_stream", "(Lorg/andglk/Stream;[FIXME: char *]J)V");

	(*env)->CallVoidMethod(env, _this, mid, str, buf, len);

}

void glk_set_style(glui32 styl)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "set_style", "(J)V");

	(*env)->CallVoidMethod(env, _this, mid, (jlong) styl);

}

void glk_set_style_stream(strid_t str, glui32 styl)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "set_style_stream", "(Lorg/andglk/Stream;J)V");

	(*env)->CallVoidMethod(env, _this, mid, str, styl);

}

glsi32 glk_get_char_stream(strid_t str)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "get_char_stream", "(Lorg/andglk/Stream;)I");

	return (*env)->CallIntMethod(env, _this, mid, str);

}

glui32 glk_get_line_stream(strid_t str, char *buf, glui32 len)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "get_line_stream", "(Lorg/andglk/Stream;[FIXME: char *]J)J");

	return (*env)->CallLongMethod(env, _this, mid, str, buf, len);

}

glui32 glk_get_buffer_stream(strid_t str, char *buf, glui32 len)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "get_buffer_stream", "(Lorg/andglk/Stream;[FIXME: char *]J)J");

	return (*env)->CallLongMethod(env, _this, mid, str, buf, len);

}

void glk_stylehint_set(glui32 wintype, glui32 styl, glui32 hint,
    glsi32 val)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stylehint_set", "(JJJI)V");

	(*env)->CallVoidMethod(env, _this, mid, wintype, styl, hint, val);

}

void glk_stylehint_clear(glui32 wintype, glui32 styl, glui32 hint)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "stylehint_clear", "(JJJ)V");

	(*env)->CallVoidMethod(env, _this, mid, wintype, styl, hint);

}

glui32 glk_style_distinguish(winid_t win, glui32 styl1, glui32 styl2)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "style_distinguish", "(Lorg/andglk/Window;JJ)J");

	return (*env)->CallLongMethod(env, _this, mid, win, styl1, styl2);

}

glui32 glk_style_measure(winid_t win, glui32 styl, glui32 hint,
    glui32 *result)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "style_measure", "(Lorg/andglk/Window;JJ[FIXME: glui32 *])J");

	return (*env)->CallLongMethod(env, _this, mid, win, styl, hint, result);

}

frefid_t glk_fileref_create_temp(glui32 usage, glui32 rock)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "fileref_create_temp", "(JJ)Lorg/andglk/FileRef;");

	return (*env)->CallObjectMethod(env, _this, mid, usage, rock);

}

frefid_t glk_fileref_create_by_name(glui32 usage, char *name,
    glui32 rock)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "fileref_create_by_name", "(J[FIXME: char *]J)Lorg/andglk/FileRef;");

	return (*env)->CallObjectMethod(env, _this, mid, usage, name, rock);

}

frefid_t glk_fileref_create_by_prompt(glui32 usage, glui32 fmode, glui32 rock)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetStaticMethodID(env, _FileRef, "createByPrompt", "(III)I");

	return (frefid_t) (*env)->CallStaticIntMethod(env, _FileRef, mid, (jint) usage, (jint) fmode, (jint) rock);
}

frefid_t glk_fileref_create_from_fileref(glui32 usage, frefid_t fref,
    glui32 rock)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "fileref_create_from_fileref", "(JLorg/andglk/FileRef;J)Lorg/andglk/FileRef;");

	return (*env)->CallObjectMethod(env, _this, mid, usage, fref, rock);

}

void glk_fileref_destroy(frefid_t fref)
{
	if (!fref)
		return;

	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _FileRef, "release", "()V");

	(*env)->CallVoidMethod(env, *fref, mid);
}

frefid_t glk_fileref_iterate(frefid_t fref, glui32 *rockptr)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "fileref_iterate", "(Lorg/andglk/FileRef;[FIXME: glui32 *])Lorg/andglk/FileRef;");

	return (*env)->CallObjectMethod(env, _this, mid, fref, rockptr);

}

glui32 glk_fileref_get_rock(frefid_t fref)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "fileref_get_rock", "(Lorg/andglk/FileRef;)J");

	return (*env)->CallLongMethod(env, _this, mid, fref);

}

void glk_fileref_delete_file(frefid_t fref)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "fileref_delete_file", "(Lorg/andglk/FileRef;)V");

	(*env)->CallVoidMethod(env, _this, mid, fref);

}

glui32 glk_fileref_does_file_exist(frefid_t fref)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "fileref_does_file_exist", "(Lorg/andglk/FileRef;)J");

	return (*env)->CallLongMethod(env, _this, mid, fref);

}

static glui32 jstring2latin1(JNIEnv *env, jstring str, char *buf, glui32 maxlen)
{
	glui32 len = (*env)->GetStringLength(env, str);
	if (len > maxlen)
		len = maxlen;

	const jchar * jbuf = (*env)->GetStringChars(env, str, NULL);
	int i;
	for (i = 0; i < len; ++i)
		buf[i] = jbuf[i];

	(*env)->ReleaseStringChars(env, str, jbuf);
	return len;
}

static void event2glk(JNIEnv *env, jobject ev, event_t *event)
{
	static jfieldID window = 0;
	if (window == 0)
		window = (*env)->GetFieldID(env, _Event, "windowPointer", "I");

	event->win = (winid_t) (*env)->GetIntField(env, ev, window);

	if ((*env)->IsInstanceOf(env, ev, _LineInputEvent)) {
		event->type = evtype_LineInput;

		{
			static jfieldID line_id = 0;
			if (0 == line_id)
				line_id = (*env)->GetFieldID(env, _LineInputEvent, "line", "Ljava/lang/String;");

			jstring line = (*env)->GetObjectField(env, ev, line_id);
			event->val1 = jstring2latin1(env, line, _line_event_buf, _line_event_buf_len);
			_line_event_buf[event->val1] = 0;
			__android_log_print(ANDROID_LOG_DEBUG, "andglk.c", "got line: \"%s\"\n", _line_event_buf);
			event->val2 = 0;
		}
	}
}

void glk_select(event_t *event)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "select", "()Lorg/andglk/Event;");

	jobject ev = (*env)->CallObjectMethod(env, _this, mid, event);
	event2glk(env, ev, event);
}

void glk_select_poll(event_t *event)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "select_poll", "([FIXME: event_t *])V");

	(*env)->CallVoidMethod(env, _this, mid, event);

}

void glk_request_timer_events(glui32 millisecs)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "request_timer_events", "(J)V");

	(*env)->CallVoidMethod(env, _this, mid, millisecs);

}

void glk_request_line_event(winid_t win, char *buf, glui32 maxlen, glui32 initlen)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "request_line_event", "(Lorg/andglk/Window;Ljava/lang/String;J)V");

	_line_event_buf = buf;
	_line_event_buf_len = maxlen;

	jstring str = 0;
	jchar jbuf[initlen];

	if (initlen > 0) {
		char *it = buf;
		jchar *jt = jbuf;
		while (jt - jbuf < initlen)
			*(jt++) = *(it++);

		str = (*env)->NewString(env, jbuf, maxlen);
	}

	(*env)->CallVoidMethod(env, _this, mid, win ? *win : 0, str, (jlong) maxlen);
}

void glk_request_char_event(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "request_char_event", "(Lorg/andglk/Window;)V");

	(*env)->CallVoidMethod(env, _this, mid, win);

}

void glk_request_mouse_event(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "request_mouse_event", "(Lorg/andglk/Window;)V");

	(*env)->CallVoidMethod(env, _this, mid, win);

}

void glk_cancel_line_event(winid_t win, event_t *event)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "cancel_line_event", "(Lorg/andglk/Window;[FIXME: event_t *])V");

	(*env)->CallVoidMethod(env, _this, mid, win, event);

}

void glk_cancel_char_event(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "cancel_char_event", "(Lorg/andglk/Window;)V");

	(*env)->CallVoidMethod(env, _this, mid, win);

}

void glk_cancel_mouse_event(winid_t win)
{
	JNIEnv *env = JNU_GetEnv();
	static jmethodID mid = 0;
	if (mid == 0)
		mid = (*env)->GetMethodID(env, _class, "cancel_mouse_event", "(Lorg/andglk/Window;)V");

	(*env)->CallVoidMethod(env, _this, mid, win);

}
