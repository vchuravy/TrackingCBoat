using DataFrames
using Gadfly

df = readtable("out/positions.csv")

μ_r = mean(df[:Radius])
σ_r = std(df[:Radius])

# Remove all points that are probably bogus
cond = μ_r - σ_r .<= df[:Radius] .<= μ_r + σ_r

df = sub(df, cond)

# plot(df, x = :x, y = :y)

vx = Array(Float64, size(df, 1))
vy = Array(Float64, size(df, 1))

for i in 2:size(df, 1)
  t0 = df[i-1, :Time]
  x0 = df[i-1, :x]
  y0 = df[i-1, :y]
  t1 = df[i, :Time]
  x1 = df[i, :x]
  y1 = df[i, :y]

  Δy = y1 - y0
  Δx = x1 - x0
  Δt = t1 - t0

  vx[i] = Δx/Δt
  vy[i] = Δy/Δt
end 

vs = DataFrame(t = df[2:end, :Time], vx = vx[2:end], vy = vy[2:end])
vs[:v] = √(vs[:vx].^2 .+ vs[:vy].^2)
writetable("out/velos.csv", vs)

function mean_windowed(df, id, c, Δ)
  r = 1:Δ:size(df, 1)
  idx = Array(Float64, length(r))
  vals = Array(Float64, length(r))
  j = 1
  for i in 1:Δ:size(df, 1)
    if i+Δ < size(df, 1)
      idx[j] = mean(df[i:i+Δ, id])
      vals[j] = mean(df[i:i+Δ, c])
    else 
      idx[j] = mean(df[i:end, id])
      vals[j] = mean(df[i:end, c])
    end
    j += 1
  end
  idx, vals 
end

# plot(vs, x = :t, y = :v, Geom.line)