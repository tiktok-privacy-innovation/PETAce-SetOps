# JSON Configuration Description

Please modify the relevant parameters according to your settings.

## JSON Structure

```json
{
    "network": {
        "address": "127.0.0.1",
        "remote_port": 30331,
        "local_port": 30330,
        "timeout": 90,
        "scheme": 0
    },
    "common": {
        "ids_num": 1,
        "is_sender": true,
        "verbose": true,
        "memory_psi_scheme": "psi",
        "psi_scheme": "ecdh"
    },
    "data": {
        "input_file": "/data/sender_input_file.csv",
        "has_header": false,
        "output_file": "/data/sender_output_file.csv"
    },
    "ecdh_params": {
        "curve_id": 415,
        "obtain_result": false
    }
}
```

## Parameters

| Name                       | Property | Type   | Description                                                                  | Default Value                    |
|----------------------------|----------|--------|------------------------------------------------------------------------------|----------------------------------|
| `network`                  |          |        |                                                                              |                                  |
| &emsp; `address`           | required | string | Couterparty's ip address                                                     | `127.0.0.1`                      |
| &emsp; `remote_port`       | required | uint16 | Couterparty's Ip port.                                                       | `30330`                          |
| &emsp; `local_port`        | required | uint16 | Local ip port.                                                               | `30331`                          |
| &emsp; `timeout`           | required | uint64 | Timeout for net io.                                                          | `90`                             |
| &emsp; `scheme`            | optimal  | uint32 | Scheme of network: socket(0), grpc(1). Now we only support socket io.        | `0`                              |
| `common`                   |          |        |                                                                              |                                  |
| &emsp; `ids_num`           | required | uint64 | The number of ids column's of the sender or receiver.                        | `1`                              |
| &emsp; `is_sender`         | required | bool   | Whether sender or receiver.                                                  | `true`                           |
| &emsp; `verbose`           | required | bool   | Print logs or not.                                                           | `true`                           |
| &emsp; `memory_psi_scheme` | optimal  | string | Scheme of private set operations: psi, pjc, or pir. Now we only support psi. | `"psi"`                          |
| &emsp; `psi_scheme`        | optimal  | string | Scheme of psi. Now we only support ecdh psi.                                 | `"ecdh"`                         |
| `data`                     |          |        |                                                                              |                                  |
| &emsp; `input_file`        | optimal  | string | Sender or receiver's input file.                                             | `"/data/sender_input_file.csv"`  |
| &emsp; `has_header`        | optimal  | bool   | Whether the input file has header.                                           | `false`                          |
| &emsp; `output_file`       | optimal  | string | The path of output file to save the intersection sets.                       | `"/data/sender_output_file.csv"` |
| `ecdh_params`              |          |        |                                                                              |                                  |
| &emsp; `curve_id`          | required | uint64 | Ecc curve id in openssl.                                                     | `NID_X9_62_prime256v1(415)`      |
| &emsp; `obtain_result`     | required | bool   | Set true if the party can obatin intersection result.                        | `receiver:true, sender:false`    |
