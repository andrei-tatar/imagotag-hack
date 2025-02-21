import { MonoTypeOperatorFunction, filter, map, tap } from "rxjs";
import { splitIntoChunks } from "./split-into-chunks";
import { isDefined } from "../util";

const COBS_SEPARATOR = 0;

export function cobsEncode(): MonoTypeOperatorFunction<Buffer> {
  return (src) =>
    src.pipe(
      map((buffer) => {
        let read_index = 0;
        let write_index = 1;
        let code_index = 0;
        let code = 1;

        const encodedBuffer: number[] = [];

        while (read_index < buffer.length) {
          if (buffer[read_index] == 0) {
            encodedBuffer[code_index] = code;
            code = 1;
            code_index = write_index++;
            read_index++;
          } else {
            encodedBuffer[write_index++] = buffer[read_index++];
            code++;

            if (code == 0xff) {
              encodedBuffer[code_index] = code;
              code = 1;
              code_index = write_index++;
            }
          }
        }

        encodedBuffer[code_index] = code;
        encodedBuffer.push(COBS_SEPARATOR);
        return Buffer.from(encodedBuffer);
      })
    );
}

export function cobsDecode(): MonoTypeOperatorFunction<Buffer> {
  return (src) =>
    src.pipe(
      splitIntoChunks(COBS_SEPARATOR),
      map((chunk) => {
        let read_index = 0;
        let write_index = 0;
        let code = 0;
        let i = 0;

        const decodedBuffer: number[] = [];

        while (read_index < chunk.length) {
          code = chunk[read_index];

          if (read_index + code > chunk.length && code != 1) {
            return null;
          }

          read_index++;

          for (i = 1; i < code; i++) {
            decodedBuffer[write_index++] = chunk[read_index++];
          }

          if (code != 0xff && read_index != chunk.length) {
            decodedBuffer[write_index++] = 0;
          }
        }

        return Buffer.from(decodedBuffer);
      }),
      filter(isDefined)
    );
}
