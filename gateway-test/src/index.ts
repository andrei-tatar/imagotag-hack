import { first, interval, map, merge, mergeMap, tap, timeout } from "rxjs";
import { CobsStream } from "./communication/cobs-stream";
import { SerialStream } from "./communication/serial-stream";

const serial = new SerialStream({
  port: "/dev/ttyUSB0",
  baud: 115200,
});

const cobs = new CobsStream(serial);

interval(50)
  .pipe(
    mergeMap((index) => {
      const tx$ = cobs.tx(
        Buffer.from(
          `how about a longer message that contains an index ${++index}`
        )
      );

      const rx$ = cobs.rx$.pipe(
        map((rx) => {
          const msg = rx.toString();
          const value = +msg.split(" ").slice(-1);
          return { msg, value };
        }),
        first(({ value }) => value === index),
        timeout(30),
        tap(({ msg }) => console.log(`rx:${msg}`))
      );

      return merge(rx$, tx$);
    })
  )
  .subscribe({
    error: (err) => console.error(err),
  });
