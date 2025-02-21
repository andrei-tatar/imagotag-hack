import { Observable } from "rxjs";

export interface DataStream {
  tx(msg: Buffer): Observable<never>;
  rx$: Observable<Buffer>;
}
