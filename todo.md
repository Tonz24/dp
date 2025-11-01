# TODO:
- organize descriptor set bindings
  - introduce variables
  - put them in a file somewhere where both shaders and cpp headers can include them
  - profit (no more headaches)

- rewrite buffer allocation to raii
- rename every instance of sky to environment map
- figure out how to organize shaders (different material models, RT shader types) in a way that doesn't cause involuntary head explosion