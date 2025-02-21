import { MonoTypeOperatorFunction, concatMap, scan } from "rxjs";

export function splitIntoChunks(
  separator: number | Uint8Array
): MonoTypeOperatorFunction<Buffer> {
  const separatorLength = typeof separator === "number" ? 1 : separator.length;
  return (src) =>
    src.pipe(
      scan(
        (ctx, chunk) => {
          let pending = Buffer.concat([ctx.pending, chunk]);
          let index: number;
          let start = 0;
          ctx.send = [];

          do {
            index = pending.indexOf(separator, start);
            if (index >= 0) {
              const chunk = pending.subarray(start, index);
              if (chunk.length) {
                ctx.send.push(chunk);
              }
              start = index + separatorLength;
            } else {
              break;
            }
          } while (true);

          if (start > 0) {
            ctx.pending = pending.subarray(start);
          } else {
            ctx.pending = pending;
          }

          return ctx;
        },
        {
          pending: Buffer.from([]),
          send: [] as Buffer[],
        }
      ),
      concatMap((v) => v.send)
    );
}
