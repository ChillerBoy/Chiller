# Cleanup Report

Date: 2025-09-08T00:16:35.173245

## Actions

- Moved temporary notes to `docs/old/`: README_MERGE.txt, README_PATCH.txt
- Renamed `Seuquence_of_Operations.md` → `Sequence_of_Operations.md` (if present)
- Moved all `.cpp/.h` files into `src/`
- Moved non-README docs into `docs/`
- Moved suspected unused modules to `src/_unused/`: HMI_events, ModbusMap_ACH580, Utils, permissions

## Notes

This 'unused' detection is conservative and based on:
- header not included anywhere AND
- basename not mentioned in any other file.

If something in `src/_unused/` is actually required, simply move it back to `src/`.
