import csv, math, datetime, sys
from garmin_fit_sdk import Decoder, Stream
import fitdecode

rows=[]
with open("replay2.csv") as f:
    for r in csv.reader(f):
        rows.append(dict(t=int(r[0]),lat=float(r[1]),lon=float(r[2]),ele=float(r[3]),d=float(r[4]),sp=float(r[5]),hr=int(r[6]),cad=int(r[7])))
def expected(skip_pause, upto=None):
    out=[]
    for i,r in enumerate(rows[:upto]):
        if skip_pause and 10000<i<10100: continue
        out.append(r)
    return out
def ts(x): return x.timestamp() if isinstance(x,datetime.datetime) else x+631065600
SC=180.0/2**31

def check(path, exp, label):
    print("=== %s (%s)"%(label,path))
    st=Stream.from_file(path); dec=Decoder(st)
    print("SDK Garmin : is_fit=%s, check_integrity=%s"%(dec.is_fit(), Decoder(Stream.from_file(path)).check_integrity()))
    msgs,errs=Decoder(Stream.from_file(path)).read(apply_scale_and_offset=True,convert_datetimes_to_dates=True,merge_heart_rates=False)
    print("  erreurs de decodage :",errs)
    print("  messages :",{k:len(v) for k,v in msgs.items()})
    # fitdecode (deuxieme decodeur, independant)
    names={}
    with fitdecode.FitReader(path) as fr:
        for fr_ in fr:
            if fr_.frame_type==fitdecode.FIT_FRAME_DATA: names[fr_.name]=names.get(fr_.name,0)+1
    print("  fitdecode (CRC verifie) :",names)
    rec=msgs["record_mesgs"]; assert len(rec)==len(exp),(len(rec),len(exp))
    mx=dict(t=0,lat=0,lon=0,alt=0,dist=0,sp=0,hr=0,cad=0)
    for a,e in zip(rec,exp):
        mx["t"]=max(mx["t"],abs(ts(a["timestamp"])-e["t"]))
        mx["lat"]=max(mx["lat"],abs(a["position_lat"]*SC-e["lat"])); mx["lon"]=max(mx["lon"],abs(a["position_long"]*SC-e["lon"]))
        mx["alt"]=max(mx["alt"],abs(a["altitude"]-e["ele"])); mx["dist"]=max(mx["dist"],abs(a["distance"]-e["d"]))
        mx["sp"]=max(mx["sp"],abs(a["speed"]-e["sp"])); mx["hr"]=max(mx["hr"],abs(a["heart_rate"]-e["hr"])); mx["cad"]=max(mx["cad"],abs(a["cadence"]-e["cad"]))
        assert a.get("power") is None
    print("  ecarts max sur %d enregistrements : temps %d s, lat %.2e deg, lon %.2e deg, altitude %.2f m, distance %.2f m, vitesse %.3f m/s, FC %d, cadence %d"%(
        len(rec),mx["t"],mx["lat"],mx["lon"],mx["alt"],mx["dist"],mx["sp"],mx["hr"],mx["cad"]))
    s=msgs["session_mesgs"][0]
    # valeurs attendues
    el=exp[-1]["t"]-exp[0]["t"]
    hrs=[e["hr"] for e in exp]; cads=[e["cad"] for e in exp]
    ref=None;asc=0;desc=0
    for e in exp:
        if ref is None: ref=e["ele"]
        elif e["ele"]-ref>=1.0: asc+=e["ele"]-ref; ref=e["ele"]
        elif ref-e["ele"]>=1.0: desc+=ref-e["ele"]; ref=e["ele"]
    print("  SESSION : sport=%s, duree ecoulee %.0f s (attendu %d), duree minuterie %.0f s, distance %.1f m (attendu %.1f)"%(
        s["sport"],s["total_elapsed_time"],el,s["total_timer_time"],s["total_distance"],exp[-1]["d"]-exp[0]["d"]))
    print("            FC moy %s (attendu %.1f) max %s (attendu %d), cadence moy %s (attendu %.1f) max %s, puissance %s, vitesse moy %.3f max %.3f"%(
        s["avg_heart_rate"],sum(hrs)/len(hrs),s["max_heart_rate"],max(hrs),s["avg_cadence"],sum(cads)/len(cads),s["max_cadence"],s.get("avg_power"),s["avg_speed"],s["max_speed"]))
    print("            D+ %s m (attendu %.0f avec le meme seuil de 1 m), D- %s m (attendu %.0f), tours %s"%(s["total_ascent"],asc,s["total_descent"],desc,s["num_laps"]))
    for l in msgs["lap_mesgs"]:
        print("  TOUR %s : %.0f s, %.1f m, FC moy %s, D+ %s m"%(l["message_index"],l["total_elapsed_time"],l["total_distance"],l["avg_heart_rate"],l["total_ascent"]))
    a=msgs["activity_mesgs"][0]
    print("  ACTIVITE : minuterie %.0f s, sessions %s, heure locale %s"%(a["total_timer_time"],a["num_sessions"],a["local_timestamp"]))
    print("  EVENEMENTS :",[(str(ts(e['timestamp'])-rows[0]['t']),e['event'],e['event_type']) for e in msgs["event_mesgs"]])

check("ride.fit",expected(True),"Sortie complete, pause de 100 s et tour a 50 km")
check("ride_recovered.fit",expected(False,6989),"Fichier recupere apres coupure")
