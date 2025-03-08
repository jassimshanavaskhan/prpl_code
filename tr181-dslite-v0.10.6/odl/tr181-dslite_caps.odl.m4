%config {
    %global privileges = {
        user = "USER_ID", group = "GROUP_ID", capabilities = ["CAP_NET_ADMIN", "CAP_DAC_OVERRIDE"]
    };
}
