#ifndef _LOCAL_H
#define _LOCAL_H

typedef struct mtm_local_undo_s mtm_local_undo_t;
typedef struct mtm_local_undo_entry_s mtm_local_undo_entry_t;

struct mtm_local_undo_s {
	mtm_local_undo_entry_t *last_entry;
	char                   *buf;
	size_t                 n;
	size_t                 size;
};	

void mtm_local_init (mtm_tx_t *tx);
void mtm_local_rollback (mtm_tx_t *tx);
void mtm_local_commit (mtm_tx_t *tx);

# define DECLARE_LOG_BARRIER(name, type, encoding)                             \
void _ITM_CALL_CONVENTION name##L##encoding (mtm_tx_t *, const type *);

FOR_ALL_TYPES(DECLARE_LOG_BARRIER, mtm_local_)
void _ITM_CALL_CONVENTION mtm_local_LB (mtm_tx_t *, const void *, size_t); 

#endif /* _LOCAL_H */
