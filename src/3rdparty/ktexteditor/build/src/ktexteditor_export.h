
#ifndef KTEXTEDITOR_EXPORT_H
#define KTEXTEDITOR_EXPORT_H

#ifdef KTEXTEDITOR_STATIC_DEFINE
#  define KTEXTEDITOR_EXPORT
#  define KTEXTEDITOR_NO_EXPORT
#else
#  ifndef KTEXTEDITOR_EXPORT
#    ifdef KF5TextEditor_EXPORTS
        /* We are building this library */
#      define KTEXTEDITOR_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define KTEXTEDITOR_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef KTEXTEDITOR_NO_EXPORT
#    define KTEXTEDITOR_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef KTEXTEDITOR_DEPRECATED
#  define KTEXTEDITOR_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef KTEXTEDITOR_DEPRECATED_EXPORT
#  define KTEXTEDITOR_DEPRECATED_EXPORT KTEXTEDITOR_EXPORT KTEXTEDITOR_DEPRECATED
#endif

#ifndef KTEXTEDITOR_DEPRECATED_NO_EXPORT
#  define KTEXTEDITOR_DEPRECATED_NO_EXPORT KTEXTEDITOR_NO_EXPORT KTEXTEDITOR_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef KTEXTEDITOR_NO_DEPRECATED
#    define KTEXTEDITOR_NO_DEPRECATED
#  endif
#endif

#endif /* KTEXTEDITOR_EXPORT_H */
