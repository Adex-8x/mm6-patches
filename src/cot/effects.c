#include <pmdsky.h>
#include <cot.h>

// Internal dispatch code for item and move effects and special processes to C.
// This function is called in trampolines.s.

int cotInternalDispatchScriptSpecialProcessCall(
        struct script_routine* routine, uint32_t special_process_id, short arg1, short arg2
) {
    // TODO: arg2 doesn't seem to match the argument in the script engine?
    COT_LOGFMT(COT_LOG_CAT_SPECIAL_PROCESS, "Running special process %d (arg1=%d, arg2=%d)",
               special_process_id, arg1, arg2);

    int return_val = 0;
    bool handled = CustomScriptSpecialProcessCall(routine, special_process_id, arg1, arg2, &return_val);
    if (!handled) {
      // Log a warning that the special processed went unhandled.
      COT_WARNFMT(COT_LOG_CAT_SPECIAL_PROCESS, "Unhandled special process ID %d", special_process_id);
    }
    return return_val;
}
