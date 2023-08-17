# PSI Examples

## Quick Start

We provide two pairs of scripts (under ["scripts"](scripts) directory) to help users run our protocols in PETAce-SetOps with different settings.

To run as Party A (a sender):

```bash
bash build/example/scripts/ecdh_psi_sender_example.sh
bash build/example/scripts/ecdh_psi_sender_use_file_data.sh
```

To run as Party B (a receiver):

```bash
bash build/example/scripts/ecdh_psi_receiver_example.sh
bash build/example/scripts/ecdh_psi_receiver_use_file_data.sh
```

| Party A                            | Party B                              | Description                                                                                                                                                                                                                                                                  |
|------------------------------------|--------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| "ecdh_psi_sender_example.sh"       | "ecdh_psi_receiver_example.sh"       | An example of ECDH-PSI using random data.                                                                                                                                                                                                                                    |
| "ecdh_psi_sender_use_file_data.sh" | "ecdh_psi_receiver_use_file_data.sh" | An example of ECDH-PSI using file data. Before running this script, please change the `input_file` and `output_file` of the JSON configuration of [Party A](json/ecdh_psi_sender.json) and [Party B](json/ecdh_psi_receiver.json) to the correct absolute path of the files. |

Please refer to [Scripts Description](scripts/README.md) for more details about parameters description.

## Example Applications

### Aviation Safety

Airline companies and Interpol cooperatively check if any passage on each flight should be denied boarding or disembarkation based on the Terror Watch List.
This check can be performed by simply revealing the airline's entire passenger list to Interpol.
However, revealing this information may result in privacy risks for innocent passengers.

To address privacy implications, the airline company and Interpol can run a Private Set Intersection (PSI) protocol between the passenger list and the Terror Watch List.
In this privacy-preserving manner, the airline and Interpol only learn the information containing passengers on the Terror Watch List, without disclosing any other information to either party.

Consider the following toy example. Assume each passenger's information is structured as `(name, passport)`.
The airline's passenger list is `{(Bob, 123456789), (Charlie, 178876536)}`, while the Interpol's Terror Watch List is`{(Bob,123456789), (David, 189141556), (Frank, 138517917), ..., (Kate, 175912635)}`.
After the execution of the PSI protocol, the airline, and Interpol will know that `(Bob, 123456789)` is the intersection and indicates that Bob is a terrorist.

### Healthcare Research

Suppose there are two healthcare institutions: hospital X and research institute Y.
They both have their own patient data indexed by a personally unique identifier (e.g., a social security number).

Now, in order to conduct a specific disease study, research institute Y wants to identify individuals who are also patients of hospital X, and then conduct disease statistics and analysis based on common patients.
This means they need to find the intersection of two data sets first. However, due to data privacy concerns, they are unable to directly exchange entire patient lists.

One potential solution is to run a Private Set Intersection (PSI) protocol between hospital X and research institute Y.
In this privacy-preserving manner, research institute Y and hospital X only learn the intersection between their patient list, without disclosing any patient identities that are not in the intersection to both parties.

Consider the following toy example. Assume each patient's information is structured as `(name, social security number)`.
Hospital X has its patient list as `{(Bob, 520-09-0437), (Charlie, 009-32-1945)}`, and the patient list in research institute Y is represented as `{(Bob, 520-09-0437), (David, 528-74-4391), (Frank, 504-44-8029), ..., (Kate, 036-18-3944)}`.
After the execution of the PSI protocol, research institute Y knows that `(Bob, 520-09-0437)` is the intersection and indicates that Bob is also a patient.

### Run the PSI Protocol Using csv Files as Input

We generate some synthetic data to simulate the scenario of Aviation Safety.
We save the airline's passenger list in ["airline_passenger_list.csv"](data/airline_passenger_list.csv) and the Interpol's Terror Watch List in ["interpol_terror_watch_list.csv"](data/interpol_terror_watch_list.csv).

First, change the `input_file` and `output_file` of the JSON configuration of [Party A](json/ecdh_psi_sender.json) to the absolute path of ["airline_passenger_list.csv"](data/airline_passenger_list.csv) (`${PETAce-Setops}/example/data/airline_passenger_list.csv`) and the output file path (`${PETAce-Setops}/example/data/sender_output_list.csv`) respectively.

Then, change the `input_file` and `output_file` of the JSON configuration of [Party B](json/ecdh_psi_receiver.json) to the absolute path of ["interpol_terror_watch_list.csv"](data/interpol_terror_watch_list.csv) (`${PETAce-Setops}/example/data/interpol_terror_watch_list.csv`) and the output file path (`${PETAce-Setops}/example/data/receiver_output_list.csv`) respectively.

There is a slight difference between Aviation Safety and Healthcare Research.
In Aviation Safety, both the airline and Interpol need to know the intersection, but in Healthcare Research only the research institute needs to know the intersection.
The other party, hospital, can leave the `output_file` unset and set `obtain_result`of the JSON configuration to `false`.

Lastly, run the following two scripts in two separate terminals.

Party A (Airline):

```bash
bash build/example/scripts/ecdh_psi_sender_use_file_data.sh
```

Party B (Interpol):

```bash
bash build/example/scripts/ecdh_psi_receiver_use_file_data.sh
```

The intersection result `"123456789"` will be saved in both files (`${PETAce-Setops}/example/data/receiver_output_list.csv`) and (`${PETAce-Setops}/example/data/sender_output_list.csv`).

## Sender/Receiver Configuration

Please refer to [JSON Configuration Description](json/README.md) for more details about the JSON configuration for sender and receiver.
