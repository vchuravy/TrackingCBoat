using DataFrames

Base.convert(::Type{Float64}, ::NAtype) = NaN
using Gadfly

df = readtable("out/positions.csv")

μ_r = mean(df[:Radius])
σ_r = std(df[:Radius])

# petri dish
pR = 337.586273 #pixel
px = 365.500000 #pixel
py = 353.500000 #pixel
pDcm = 25 # cm
cboatD = 0.3 #cm
ratioCboat = μ_r/(0.5*cboatD)
ratioPetri = pR/(0.5*pDcm)

rInfluence = 2.5
rValid = 0.5(pDcm - 2rInfluence)*ratioPetri

df[:x] = df[:x] - px 
df[:y] = df[:y] - py

radius(x, y) = √((x)^2 + (y)^2)
#polar coordinates
df[:r] = map(radius, df[:x], df[:y])
df[:ϕ] = map((x, y) -> atan2(x,y), df[:x], df[:y])

checkValid(x, y) = radius(x, y) < rValid
df[:valid] = map(checkValid, df[:x], df[:y])

# Remove all points that are probably bogus
cond = μ_r - σ_r .<= df[:Radius] .<= μ_r + σ_r

df = sub(df, cond)
dfValid = sub(df, df[:valid])

# plot(df, x = :x, y = :y)

function v_a(df)
  vx = DataArray(Float64, size(df, 1))
  vy = DataArray(Float64, size(df, 1))
  ax = DataArray(Float64, size(df, 1))
  ay = DataArray(Float64, size(df, 1))

  vx[1] = NA
  vy[1] = NA
  ax[1] = NA
  ay[1] = NA

  for i in 2:size(df, 1)
    # check that frames directly follow each other
    if df[i-1, :Frame] == df[i, :Frame]-1 
      Δy = df[i, :y] - df[i-1, :y]
      Δx = df[i, :x] - df[i-1, :x]
      Δt = df[i, :Time] - df[i-1, :Time]

      vx[i] = Δx/Δt
      vy[i] = Δy/Δt

      Δvx = vx[i] - vx[i-1]
      Δvy = vy[i] - vy[i-1]

      ax[i] = Δvx/Δt
      ay[i] = Δvy/Δt
    else #  Discontinuity 
      vx[i] = NA
      vy[i] = NA 
      ax[i] = NA
      ay[i] = NA
    end
  end 

  r = DataFrame(t = df[:, :Time], vx = vx, vy = vy, ax = ax, ay = ay)
  r[:v] = √(r[:vx].^2 .+ r[:vy].^2)
  r[:a] = √(r[:ax].^2 .+ r[:ay].^2)
  return r
end

dfVA = v_a(df)
dfValidVA = v_a(dfValid)
writetable("out/va.csv", dfVA)
writetable("out/vaFiltered.csv", dfValidVA)

function mean_windowed_frames(df, id, c, Δ)
  r = 1:Δ:size(df, 1)
  idx = Array(Float64, length(r))
  vals = Array(Float64, length(r))
  j = 1
  for i in r
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

function mean_windowed_time(df, id, c, Δt)
  r = 0:Δt:maximum(df[id])
  idx = Array(Float64, length(r)-1)
  vals = Array(Float64, length(r)-1)

  j = 1
  for t0 in r
    if j < length(r)
      t1 = t0 + Δt
      idx[j] = t0 + 0.5Δt

      cond = t0 .<= df[id] .< t1
      subdf = sub(df, cond, c)

      vals[j] = mean(subdf[c])
    end
    j += 1
  end
  idx, vals 
end

# plot(vs, x = :t, y = :v, Geom.line)