
# Scripts Description

We take "ecdh_psi_sender_example.sh" as an example to illustrate the details.

## Script

```bash
BIN_DIR="@BIN_DIR@"
JSON_DIR="@JSON_DIR@"
LOG_DIR="@LOG_DIR@"

mkdir -p "${LOG_DIR}/psi/ecdh_psi/example/balanced"
mkdir -p "${LOG_DIR}/psi/ecdh_psi/example/unbalanced"

balanced_log_path_bandwith="${LOG_DIR}/psi/ecdh_psi/example/balanced"
echo "Sender balanced test"
balanced_intersection_size_array=(500)
for(( i=0;i<${#balanced_intersection_size_array[@]};i++))
do
echo "Test intersection size ${balanced_intersection_size_array[i]}"
"${BIN_DIR}/setops_example" --config_path="${JSON_DIR}/ecdh_psi_sender.json" --log_path=$balanced_log_path_bandwith --use_random_data=true --intersection_size=${balanced_intersection_size_array[i]} --intersection_ratio=2
done

unbalanced_log_path_bandwith="${LOG_DIR}/psi/ecdh_psi/example/unbalanced"
echo "Sender unbalanced test"
unbalanced_intersection_size_array=(10)
for(( i=0;i<${#unbalanced_intersection_size_array[@]};i++))
do
echo "Test intersection size ${unbalanced_intersection_size_array[i]}"
"${BIN_DIR}/setops_example" --config_path="${JSON_DIR}/ecdh_psi_sender.json" --log_path=$unbalanced_log_path_bandwith --use_random_data=true --intersection_size=${unbalanced_intersection_size_array[i]} --intersection_ratio=100
done
```

## Parameters

| Name                 | Property                           | Type   | Description                                                  | Default Value                   |
|----------------------|------------------------------------|--------|--------------------------------------------------------------|---------------------------------|
| `config_path`        | required                           | string | The path where the sender's config file located.             | `"./json/ecdh_psi_sender.json"` |
| `use_random_data`    | required                           | bool   | Use randomly generated data or read data from files.         | `true`                          |
| `log_path`           | optimal                            | string | The directory where log file located.                        | `"./logs/"`                     |
| `intersection_size`  | required if use_random_data = true | uint64 | The intersection size of both party.                         | `10`                            |
| `intersection_ratio` | required if use_random_data = true | uint64 | The ratio of sender/receiver data size to intersection size. | `100`                           |
