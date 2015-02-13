using DataFrames

concentrations = sort([[0:8]//8, 1//16, 15//16, 13//16, 3//16])
target = 800 # MicroLitre

mixtures = map(concentrations) do c
  float(c*target), float(target - c*target)
end

cs = [zip(mixtures...)...]
df = DataFrame(CAcid = [cs[1]...], Water = [cs[2]...])
writetable("mixtures.csv", df)
