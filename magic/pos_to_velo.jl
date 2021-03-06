using DataFrames

Base.convert(::Type{Float64}, ::NAtype) = NaN
using Gadfly

mad(X) = mad(X, median(X))
mad(X, med) = median([abs(x - med) for x in X])
spread(X) = maximum(x) - minimum(X)
radius(x, y) = √((x)^2 + (y)^2)

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
  r[:s] = √(r[:vx].^2 .+ r[:vy].^2)
  r[:a] = √(r[:ax].^2 .+ r[:ay].^2)
  r[:s_avg] = moving_avg(r[:s], 120) #at 30fps  => 4s
  r[:a_avg] = moving_avg(r[:a], 120)
  return r
end

function processData(folder, fileN)
  df = readtable("$folder/$fileN")
  dfOld = df

  median_r = median(head(df[:Radius]))

  # petri dish
  pR, px, py = readdlm("$folder/petriDish.txt")[1, 2:end]
  pDcm = 25 # cm
  cboatD = 0.3 #cm
  ratioCboat = median_r/(0.5*cboatD)

  rInfluence = 3.5 #cm
  rValid = pR/ratioCboat - rInfluence

  checkValid(x, y) = radius(x, y) < rValid

  df[:x] = (df[:x] - px)/ratioCboat 
  df[:y] = (df[:y] - py)/ratioCboat

  #polar coordinates
  # df[:r] = map(radius, df[:x], df[:y])
  # df[:ϕ] = map((x, y) -> atan2(x,y), df[:x], df[:y])

  print("Recorded $(size(df, 1)) data points")
  # Remove all points that are probably bogus
  # cond = median_r - 2σ_r .<= df[:Radius] .<= median_r + 2σ_r

  duplicates = Int64[]
  for subdf in groupby(df, :Frame)
      if size(subdf, 1) > 1
          push!(duplicates, subdf[1,:Frame])
      end
  end

  cond = Bool[x ∉ duplicates for x in df[:Frame]]

  dfDiscarded = sub(df, !cond)
  df = sub(df, cond)
  print(" $(size(df, 1)) of them are valid and")
  # remove all points that are in the influence of the border
  dfValid = sub(df, map(checkValid, df[:x], df[:y]))
  println(" $(size(dfValid, 1)) are outside the influence of the boundary.")
  # plot(df, x = :x, y = :y)

  dfVA = v_a(df)
  dfValidVA = v_a(dfValid)
  writetable("$folder/va.csv", dfVA)
  writetable("$folder/vaFiltered.csv", dfValidVA)

  df, dfOld, dfVA, dfValidVA
end

function moving_avg(X :: DataArray, window :: Integer)
  avg = similar(X, Float64)
  if isodd(window)
    step = div(window - 1, 2)
  else 
    step = div(window, 2)
  end

  for i in 1:size(X,1)
    if isna(X[i])
      avg[i] = NA
    else
      sum = 0.0
      n = 0
      for j in i-step:i+step    
        if (1 <= j <= size(X,1)) && (!isna(X[j]))
          sum += X[j]
          n += 1
        end
      end
      avg[i] = sum / n
    end
  end
  return avg
end

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