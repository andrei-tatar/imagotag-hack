import { Observable, filter, map, of, share, switchMap } from "rxjs";
import { DataStream } from "./types";
import { cobsDecode, cobsEncode } from "./cobs";
import { crc16 } from "./crc";
import { isDefined } from "../util";

export class CobsStream implements DataStream {
  readonly rx$: Observable<Buffer>;

  constructor(private readonly inner: DataStream, private withCrc = false) {
    let cobs$ = this.inner.rx$.pipe(cobsDecode());

    if (withCrc) {
      cobs$ = cobs$.pipe(
        map((rx) => {
          const msg = rx.subarray(0, rx.length - 2);
          const computedCrc = crc16(msg);
          const packageCrc = rx.readUint16BE(rx.length - 2);
          if (computedCrc == packageCrc) {
            return msg;
          }
          return null;
        }),
        filter(isDefined)
      );
    }

    this.rx$ = cobs$.pipe(share());
  }

  tx(msg: Buffer): Observable<never> {
    return of(msg).pipe(
      map((msg) => {
        if (this.withCrc) {
          const crc = crc16(msg);
          const withCrc = Buffer.alloc(msg.length + 2);
          msg.copy(withCrc);
          withCrc.writeUInt16BE(crc, msg.length);
          return withCrc;
        }
        return msg;
      }),
      cobsEncode(),
      switchMap((msg) => this.inner.tx(msg))
    );
  }
}
