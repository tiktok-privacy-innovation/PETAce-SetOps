# Copyright 2023 TikTok Pte. Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from petace.setops.pysetops import psi, NetParams, NetScheme
from petace.setops.pysetops import PSIScheme


class PSI:
    """
    PSI utils, now support ECDH psi and KKRT psi.

    Both parties input their sets and get the intersection of their sets.

    Attributes:
    party_id: The party id of the set owner.
    net_params: The network params.
    net_type: The network type.
    psi_scheme: Which psi scheme will use.

    Args:
    party_id (int): The party id of the set owner.
    net_params (NetParams): The network params.
    net_scheme (NetScheme): The network type.
    psi_scheme (PSIScheme): Which psi scheme will use.
    """

    party_id = 0
    net_params = None
    net_scheme = None
    psi_scheme = None

    def __init__(self, party_id: int, net_params: "NetParams", net_scheme: "NetScheme",
                 psi_scheme: "PSIScheme") -> None:
        self.party_id = party_id
        self.net_params = net_params
        self.net_scheme = net_scheme
        self.psi_scheme = psi_scheme

    def process(self, input: list[str], obtain_result: bool, verbose: bool = False) -> list[str]:
        """
        Process psi protocol.

        Parameters:
        input (list of str): The set of each party.
        obtain_result (bool): Whether this party get the intersection.
        verbose (bool): Whether print logs.

        Returns:
        list of str: The intersection if obtain_result is true, else empty list.
        """
        return psi(input, self.party_id, obtain_result, verbose, self.net_params, self.net_scheme, self.psi_scheme)
