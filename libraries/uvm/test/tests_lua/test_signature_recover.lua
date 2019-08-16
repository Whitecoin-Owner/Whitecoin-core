print("test signature recover")
let sig1 = "1bfc992468ae2d0fc3d567813c5eb9e7e874db4ed2b2d08a8327216a9934034de6d3081c5f7901f3e897a7b4c4e09dcb59d2e8b7455d4d6c6dfabed227ecefd666"
let digest1 = "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"
let a1 = signature_recover(sig1, digest1)
print("a1=", a1)
