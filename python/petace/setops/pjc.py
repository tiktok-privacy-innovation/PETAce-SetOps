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

from petace.setops.pysetops import pjc, NetParams, NetScheme
from petace.setops.pysetops import PJCScheme


class PJC:
    """
    PJC utils, now only support Circuit psi.

    Both parties input their sets and get the share of the intersection of their sets.

    Attributes:
    party_id: The party id of the set owner.
    net_params: The network params.
    net_type: The network type.
    pjc_scheme (PJCScheme): Which psi scheme will use.

    Args:
    party_id (int): The party id of the set owner.
    net_params (NetParams): The network params.
    net_scheme (NetScheme): The network type.
    pjc_scheme (PJCScheme): Which psi scheme will use.
    """

    party_id = 0
    net_params = None
    net_scheme = None
    pjc_scheme = None

    def __init__(self, party_id: int, net_params: "NetParams", net_scheme: "NetScheme",
                 pjc_scheme: "PJCScheme") -> None:
        self.party_id = party_id
        self.net_params = net_params
        self.net_scheme = net_scheme
        self.pjc_scheme = pjc_scheme

    def process(self, keys: list[str], features: list[list[int]], verbose: bool = False) -> list[list[int]]:
        """
        Process psi protocol.

        Parameters:
        keys (list of str): The master ke.
        features (tow dim int list): The features corresponding to keys.
        verbose (bool): Whether print logs.

        Returns:
        tow dim int list: The share of features and whether they in the intersection.
        """
        return pjc(keys, features, self.party_id, verbose, self.net_params, self.net_scheme, self.pjc_scheme)
