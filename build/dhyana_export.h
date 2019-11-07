
#ifndef DHYANA_EXPORT_H
#define DHYANA_EXPORT_H

#ifdef DHYANA_STATIC_DEFINE
#  define DHYANA_EXPORT
#  define DHYANA_NO_EXPORT
#else
#  ifndef DHYANA_EXPORT
#    ifdef dhyana_EXPORTS
        /* We are building this library */
#      define DHYANA_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define DHYANA_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef DHYANA_NO_EXPORT
#    define DHYANA_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef DHYANA_DEPRECATED
#  define DHYANA_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef DHYANA_DEPRECATED_EXPORT
#  define DHYANA_DEPRECATED_EXPORT DHYANA_EXPORT DHYANA_DEPRECATED
#endif

#ifndef DHYANA_DEPRECATED_NO_EXPORT
#  define DHYANA_DEPRECATED_NO_EXPORT DHYANA_NO_EXPORT DHYANA_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef DHYANA_NO_DEPRECATED
#    define DHYANA_NO_DEPRECATED
#  endif
#endif

#endif /* DHYANA_EXPORT_H */
