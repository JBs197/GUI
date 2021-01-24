#ifndef JBSQL_GLOBAL_H
#define JBSQL_GLOBAL_H

#if defined(JBSQL_LIBRARY)
#  define JBSQL_EXPORT Q_DECL_EXPORT
#else
#  define JBSQL_EXPORT Q_DECL_IMPORT
#endif

#endif // JBSQL_GLOBAL_H
